#include "../h/exceptions.h"

/************************** EXCEPTIONS.C *********************************
* 
*  The implementation for the exception handling module of the operating system.
*
*  Description:
*  The module provides handlers for processor exceptions, including SYSCALLs,
*  TLB management, program traps, and hardware interrupts. The "Pass Up or Die" mechanism 
*  ensures that unhandled exceptions are either processed by a higher-level support 
*  structure or result in process termination.
*
*  Implementation:
*  1. exceptionHandler - The main dispatcher that classifies exceptions and
*     delegates them to specific handlers.
*  2. Dedicated handlers for each exception type:
*     - interruptHandler: Handles device interrupts (disk, network, terminals, timers).
*     - tlbExceptionHandler: Manages memory access violations due to missing 
*       or invalid TLB entries.
*     - programTrapHandler: Catches illegal operations, invalid memory accesses, 
*       and CPU errors.
*     - syscallHandler: Processes system calls (SYS1-SYS8) requested by user programs.
*  3. Individual system call implementations that provide essential OS services.
*  4. Exception Handling Behavior:
*  - The "Pass Up or Die" model determines how exceptions are handled:
*    - If a process has a support structure, the exception is passed up to 
*      the support level.
*    - If no support structure exists, the process is terminated to prevent
*      system instability.
*  - System calls issued from user-mode processes that request privileged
*    operations result in a program trap.
*
*  System Calls:
*  - SYS1: createProcess - Creates a new process with specified state
*  - SYS2: terminateProcess - Recursively terminates a process and its progeny
*  - SYS3: passeren - P operation on semaphores: decrements and blocks if negative
*  - SYS4: verhogen - V operation on semaphores: increments and unblocks waiters
*  - SYS5: waitIO - Blocks process pending I/O completion on specified device
*  - SYS6: getCPUTime - Returns accumulated CPU time used by current process
*  - SYS7: waitClock - Blocks process until the next clock tick
*  - SYS8: getSupportData - Retrieves pointer to support structure
*
*  Written by: Khoa Ho & Hieu Tran
*  March 2025
****************************************************************************/


/***************************************************************************
 * Helper function: storeState
 * Stores the current exception state in the current process's state.
 * Updates the PC to the next instruction and copies the exception state.
 * 
 * Parameters:
 * oldState: Pointer to the state to be stored
 */
HIDDEN void storeState(state_PTR oldState) {
    oldState->s_pc += WORDLEN;  /* Increment PC to next instruction */
    copyState(oldState, &(currentProcess->p_s));  /* Copy state to current process */
}

/***************************************************************************
 * Function: exceptionHandler
 * Main exception handler - Entry point for all exceptions.
 * Determines exception type by examining the cause register and
 * dispatches to the appropriate specialized handler.
 */
void exceptionHandler() {    
    state_t *excState = (state_t *) BIOSDATAPAGE;  
    int excCode = CAUSE_GET_EXCCODE(excState->s_cause); 

    if (excCode == 0)  {
        interruptHandler(excState);  
    }
    else if (excCode <= TLBINVLDL) {
        tlbExceptionHandler(); 
    } 
    else if (excCode == SYSCALL_EXCEPTION) {
        syscallHandler(excState);  
    }
    else {
        passUpOrDie(GENERALEXCEPT);  /* Handle other exceptions (program traps) */
    }
}

/***************************************************************************
 * Function: syscallHandler
 * System call handler - Processes SYSCALL exceptions.
 * Checks privilege level and dispatches to the appropriate
 * system call handler based on the system call code in a0.
 */
void syscallHandler() {
    state_t *excState = (state_t *) BIOSDATAPAGE;  
    int syscallCode = excState->s_a0;  /* Extract syscall code from a0 */
    
    /* Check if in user mode and attempting privileged syscalls */
    if (currentProcess->p_s.s_status & KUPON && syscallCode <= 8) {
        excState->s_cause = (excState->s_cause) & RICODE; /* set cause.ExcCode bits to RI */
        programTrapHandler(); 
    }
    
    /* Dispatch to appropriate syscall handler */
    switch (syscallCode) {
        case CREATEPROCESS: {
            createProcess();  /* SYS1 */
        }
        case TERMPROCESS: {
            terminateProcess(currentProcess);  /* SYS2 */
        } 
        case PASSEREN: {
            passeren();  /* SYS3 */
        }
        case VERHOGEN: {
            verhogen();  /* SYS4 */
        }
        case WAITFORIO: {
            waitIO();  /* SYS5 */
        }
        case GETCPUTIME: {
            getCPUTime();  /* SYS6 */
        }
        case WAITFORCLOCK: {
            waitClock();  /* SYS7 */
        }
        case GETSUPPORT: {
            getSupportData();  /* SYS8 */
        }
        default: {			
            passUpOrDie(GENERALEXCEPT);  /* Unknown syscall - pass up or terminate */
        }
    }
}

/***************************************************************************
 * Function: createProcess (SYS1)
 * Creates a new process as a child of the current process.
 * The new process inherits its state from a1 and optional
 * support structure from a2.
 */
void createProcess() {
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    pcb_PTR newProc = allocPcb();  /* Allocate PCB for new process */
    
    if (newProc == NULL) {
        oldState->s_v0 = -1;  /* Return error if no free PCBs */
    } else {
        /* Set the new process state to state stored in a1 */
        state_t *toCheck = (state_t*) oldState->s_a1;
        copyState(toCheck, &newProc->p_s);

        /* Copying over the support struct if provided */
        support_t *supportData = (support_t*) oldState->s_a2;
        if (supportData != NULL) {
            newProc->p_supportStruct = supportData;
        }

        /* Initialize process and add to ready queue */
        processCount++;
        insertProcQ(&readyQueue, newProc);
        insertChild(currentProcess, newProc);  /* Make child of current process */
        oldState->s_v0 = 0;  /* Success return value */
    }
    storeState(oldState);  
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: terminateProcess (SYS2)
 * Recursively terminates a process and all its children.
 * Handles cleanup of semaphore blocks, process counts,
 * and PCB resources.
 * 
 * Parameters:
 * current: Pointer to the PCB to terminate
 */
void terminateProcess(pcb_PTR current) {
    /* Recursively terminate all children first */
    while (emptyChild(current) == FALSE) {
        terminateProcess(removeChild(current));
    }

    /* Handle blocked processes and semaphore adjustment */
    if ((current->p_semAdd) != NULL) {
        if ((current->p_semAdd >= &deviceSemaphores[0]) && (current->p_semAdd <= &deviceSemaphores[CLOCK])) {
            /* If the process is blocked on a device semaphore, remove it
            from the blocked queue and decrement the soft block count */
            softBlockCount--;  
        } else {
            int *semAddress = current->p_semAdd;
            (*semAddress) += 1; 
        }
        pcb_PTR p = outBlocked(current);  /* Remove from blocked queue */
        if(p != mkEmptyProcQ()) {
            processCount--;  
        }
    } else {
        /* If not blocked, remove from ready queue */
        pcb_PTR p = outProcQ(&readyQueue, current);
        if (p != mkEmptyProcQ()) {
            processCount--;  
        }
    }

    /* Special handling if terminating the current process */
    if (current == currentProcess) {
        if (!emptyChild(current))  {
            outChild(currentProcess);  
        }
        currentProcess = mkEmptyProcQ();  
        processCount--;  
        scheduler();  
    }
    freePcb(current);  /* Return PCB to free list */
}

/***************************************************************************
 * Function: passeren (SYS3)
 * Performs P operation on semaphore specified in a1.
 * If semaphore becomes negative, blocks the current process.
 */
void passeren() { 
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    cpu_t TOD_current;
    STCK(TOD_current);  /* Get current time */
    int *semAddress = (int *) oldState->s_a1;   /* Get semaphore address from a1 */

    (*semAddress)--;  

    storeState(oldState);  
    if (*semAddress < 0) {
        /* If semaphore negative, block process */
        insertBlocked(semAddress, currentProcess);  /* Add to blocked queue */
        currentProcess->p_time += (TOD_current - TOD_start);  /* Update CPU time */
        currentProcess = NULL;  /* Clear current process */
        scheduler(); 
    }
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: verhogen (SYS4)
 * Performs V operation on semaphore specified in a1.
 * If processes are blocked on the semaphore, unblocks one.
 */
void verhogen() { 
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    int *semAddress = (int *) oldState->s_a1;  
    (*semAddress)++; 
    pcb_PTR p;
    if (*semAddress <= 0) {
        /* If processes waiting, unblock one */
        p = removeBlocked(semAddress);  /* Remove first blocked process */
        if (p != NULL) {
            insertProcQ(&readyQueue, p);  /* Add to ready queue */
        }
    }
    storeState(oldState); 
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: waitIO (SYS5)
 * Blocks the current process pending I/O completion on
 * the specified device. Updates device semaphores and
 * soft block count.
 */
void waitIO() {
    cpu_t TOD_stop;
    STCK(TOD_stop);  /* Get current time */
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    
    /* Get device parameters from registers */
    int line = oldState->s_a1;  /* Interrupt line */
    int dev = oldState->s_a2;   /* Device number */
    int term_flag = oldState->s_a3;  /* Terminal flag (1 = write, 0 = read ) */

    /* Calculate semaphore index based on device type */
    int semIndex;
    if (term_flag && line == TERMINT) {
        /* Terminal device - write operation */
        semIndex = dev + (line - DISKINT) * DEVPERINT + DEVPERINT; 
    } else {
        /* Non-terminal device or terminal read */
        semIndex = dev + (line - DISKINT) * DEVPERINT;             
    }

    deviceSemaphores[semIndex]--; 
    if(deviceSemaphores[semIndex] < 0) {
        /* If semaphore negative, block process */
        storeState(oldState);  
        currentProcess->p_time += (TOD_stop - TOD_start);  /* Update CPU time */
        insertBlocked(&(deviceSemaphores[semIndex]), currentProcess);  /* Block on device */
        
        currentProcess = mkEmptyProcQ();  /* Clear current process */
        softBlockCount++;  /* Increment soft block count */
        scheduler();  /* Call scheduler to select next process */
    }
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: getCPUTime (SYS6)
 * Returns the accumulated CPU time for the current process
 * in microseconds. Places the value in v0 register.
 */
void getCPUTime() {
    cpu_t TOD_current;
    STCK(TOD_current);  /* Get current time */
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    
    /* Calculate total CPU time: accumulated + current interval */
    oldState->s_v0 = currentProcess->p_time + TOD_current - TOD_start;

    storeState(oldState);  
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: waitClock (SYS7)
 * Blocks the current process until the next interval timer
 * interrupt (100ms). Updates the pseudo-clock semaphore and
 * soft block count.
 */
void waitClock() {
    int TOD_stop;
    STCK(TOD_stop);  /* Get current time */
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  

    (deviceSemaphores[CLOCK])--;  /* Decrement pseudo-clock semaphore */

    if (deviceSemaphores[CLOCK] >= 0) {
        /* If semaphore non-negative, no blocking needed */
        loadNextState(currentProcess->p_s);  
    }
    
    /* Block the process on the pseudo-clock semaphore */
    softBlockCount++;  
    storeState(oldState);  
    currentProcess->p_time += (TOD_stop - TOD_start);  /* Update CPU time */
    insertBlocked(&(deviceSemaphores[CLOCK]), currentProcess);  /* Block process */
    currentProcess = NULL;  /* Clear current process */

    scheduler();  
}

/***************************************************************************
 * Function: getSupportData (SYS8)
 * Returns a pointer to the current process's support structure.
 * Places the pointer in v0 register.
 */
void getSupportData() {
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;  
    oldState->s_v0 = (int) (currentProcess->p_supportStruct);  /* Return support pointer */
    storeState(oldState);  
    loadNextState(currentProcess->p_s);  /* Return control to caller */
}

/***************************************************************************
 * Function: passUpOrDie
 * Implements the Pass Up or Die policy for exceptions.
 * If the process has a support structure, passes the exception
 * up to it. Otherwise, terminates the process.
 * 
 * Parameters:
 * passUpCode: Exception type code (PGFAULTEXCEPT or GENERALEXCEPT)
 */
void passUpOrDie(int passUpCode)
{
    if ((currentProcess->p_supportStruct) != NULL) {
        /* Process has support structure - pass up the exception */
        state_PTR exceptStatePtr = (state_PTR) BIOSDATAPAGE;  
        /* Copy exception state to support structure */
        copyState(exceptStatePtr, 
                  &currentProcess->p_supportStruct->sup_exceptState[passUpCode]);  

        /* Load context from support structure */
        LDCXT(currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_stackPtr, 
              currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_status, 
              currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_pc);
    }
    else {
        /* No support structure - terminate the process */
        terminateProcess(currentProcess); 
        scheduler(); 
    }
}

/***************************************************************************
 * Function: tlbExceptionHandler
 * Handles TLB-related exceptions via the Pass Up or Die mechanism
 * with PGFAULTEXCEPT code.
 */
/* void tlbExceptionHandler() {
    passUpOrDie(PGFAULTEXCEPT);  
} */

/***************************************************************************
 * Function: programTrapHandler
 * Handles program trap exceptions via the Pass Up or Die mechanism
 * with GENERALEXCEPT code.
 */
/* void programTrapHandler() {
    passUpOrDie(GENERALEXCEPT);
} */

/***************************************************************************
 * Function: uTLB_RefillHandler
 * Handles TLB refill exceptions by loading the missing page
 * into the TLB and returning control to the process.
 */
void uTLB_RefillHandler() {
    state_PTR exceptionState = (state_PTR) BIOSDATAPAGE;  
    int vpn = (exceptionState->s_entryHI & VPNMASK) >> VPNSHIFT;  /* Extract VPN */
    vpn %= MAXPAGES;  /* Normalize VPN */

    /* Get page table entry from current process */
    support_t *supportPtr = currentProcess->p_supportStruct;

    setENTRYHI(supportPtr->sup_privatePgTbl[vpn].entryHI);  /* Set entry HI */
    setENTRYLO(supportPtr->sup_privatePgTbl[vpn].entryLO);  /* Set entry LO */

    TLBWR();  /* Write to TLB */

    LDST(exceptionState);  /* Return control to the process */
}