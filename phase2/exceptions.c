#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/initial.h"
#include "../h/exceptions.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

HIDDEN cpu_t currentTime;

void exceptionHandler() {    
    /* Get the exception state and code from the BIOS data page */
    state_t *excState = (state_t *) BIOSDATAPAGE;
    int excCode = (CAUSE_GET_EXCCODE(excState->s_cause));

    switch (excCode) {
        /* code 0 */
        case(INTERRUPTS):
            interruptHandler();
            break;
        /* code 1-3 */
        case(TLBMOD): 
        case(TLBINVLD):
        case(TLBINVLDL):
            tlbExceptionHandler(excState);
            break;

        /* code 4-7, 9-12 */
        case(ADDRINVLD):
        case(ADDRINVLDS):
        case(BUSINVLD):
        case(BUSINVLDL):
        case(BREAKPOINT):
        case(RESERVEDINST):
        case(COPROCUNUSABLE):
        case(ARITHOVERFLOW):
            programTrapHandler(excState);
            break;

        /* code 8 */
        case(SYSCALL_EXCEPTION):
            syscallHandler(excState);
            break;
    }
}

void syscallHandler(state_t *excState)
{
    int syscallCode = excState->s_a0;
    
    /*Checks KUP to see if process is in user mode*/
    if(excState->s_status & KUPON && syscallCode <= 8)
    {
        /*Attempted a syscall in user mode, trigger a program trap*/
        programTrapHandler(excState);
    }
    
    /*Switch case using parameter value to determine current syscall code, then calls other methods to act*/
    switch(syscallCode) 
    {
        /*SYS1 Create Process*/
        case CREATEPROCESS: {
            /*Creates a new process for use*/
            createProcess(excState); 
            break;}
            
        /*SYS2 Terminate Process*/	
        case TERMPROCESS: {
            /*Terminates the current process and all of its progeny*/
            terminateProcess(currentProcess); 
            break;}
            
        /*SYS3 Passaren*/
        case PASSEREN: {
            /*Perform a P operation on a semaphore*/
            passeren((int*)excState->s_a1); 
            break;}
            
        /*SYS4 Verhogen*/	
        case VERHOGEN: {
            /*Perform a V operation on a semaphore*/
            verhogen((int*)excState->s_a1); 
            break;}
            
        /*SYS5 Wait for IO*/
        case WAITFORIO: {
            /*Waits for input or output from a device*/
            waitIO(); 
            break;}
            
        /*SYS6 Get CPU Time*/
        case GETCPUT: {
            /*Stores accumulated CPU time in v0 and performs load state*/
            getCpuTime();
            break;
            }
            
        /*SYS7 Wait for Clock*/
        case WAITFORCLOCK: {
            /*Performs a P operation on the pseudo-clock semaphore and blocks the current process*/
            waitClock(); 
            break;}
            
        /*SYS8 Get Support Data*/
        case GETSUPPORTT: {
            /*Stores pointer to support structure in v0 and performs load state*/
            getSupportData();
            break;
            }
            
        /*Sycall code is greater than 8*/
        default: {			
            /*Pass up or Die*/
            passUpOrDie(GENERALEXCEPT); }
    }
    
    /*Return to current process*/
    if(currentProcess != NULL) {
        loadNextState(currentProcess->p_s);
    }
    /*No process to return to, call scheduler*/
    else{
        scheduler();
    }
}

/* sys1 no input */
void createProcess() {
    pcb_PTR newProc = allocPcb();
    
    if (newProc == NULL) {
        currentProcess->p_s.s_v0 = -1;
    } else {
        /* Set the new process state to state stored in a1 */
        state_t *toCheck = (state_t*) currentProcess->p_s.s_a1;
        copyState(toCheck, &newProc->p_s);

        /* Copying over the support struct ... */
        support_t *supportData = (support_t*) currentProcess->p_s.s_a2;

        if (supportData != NULL) {
            newProc->p_supportStruct = supportData;
        }

        /* Incrementing process count, inserting on ready queue, making a child of current process, and returning control */
        processCount++;
        insertProcQ(&readyQueue, newProc);
        insertChild(currentProcess, newProc);
        currentProcess->p_s.s_v0 = 0;
    }
    loadNextState(currentProcess->p_s);
}

/* sys2 terminate process */
void terminateProcess(pcb_PTR current) {
	while (emptyChild(current) == FALSE) {
		terminateProcess(removeChild(current));
	}
	if ((current->p_semAdd) != NULL) {
		if ((current->p_semAdd >= &deviceSemaphores[0]) && (current->p_semAdd <= &clockSem)) {
			softBlockCount--;
		} else {
            int *semAddress = current->p_semAdd;
            (*semAddress) += 1;
        }
		pcb_PTR p = outBlocked(current);
		if(p != mkEmptyProcQ()) {
			processCount--;
		}
	} else {
		pcb_PTR p = outProcQ(&readyQ, current);
        if p != mkEmptyProcQ()) {
			processCount--;
		}
	}

	if (current == currentProcess) {
		if (!emptyChild(current)) 
            outChild(currentProcess);
		currentProcess = mkEmptyProcQ();
		processCount--;    /* diff from djt */    
		scheduler();
	}
}

/* sys3 passeren */
void passeren(int *semAddress) { 
    (*semAddress)--;
    if (*semAddress < 0) {
        insertBlocked(semAddress, currentProcess);
        softBlockCount++;
        currentProcess = NULL;
        scheduler();
    }
    loadNextState(currentProcess->p_s);;
}

/* sys4 verhogen */
pcb_PTR verhogen(int *semAddress) { 
    (*semAdd)++;
    pcb_PTR p;
    if (*semAdd <= 0) {
        p = removeBlocked(semAddress);
        if (p != NULL) {
            insertProcQ(&readyQueue, p);
            softBlockCount--;
        }
    }
    loadNextState(currentProcess->p_s);;
    return p;
}

/* sys5 wait for io */
void waitIO() {
    /* Get line, device number, and possible terminal read operations */
    int line = currentProcess->p_s.s_a1;
    int dev = currentProcess->p_s.s_a2;
    int term_flag = currentProcess->p_s.s_a3;

    /* Calculate semaphore index */
    int semIndex;
    if (term_flag && line == TERMINT) {
        /* Terminal device*/
        semIndex = dev + (line - 3) * DEVPERINT + DEVPERINT; 
    } else {
        /* Non-terminal device */
        semIndex = dev + (line - 3) * DEVPERINT;             
    }

    /* Block the process */
    passeren(&deviceSemaphores[semIndex]); 
}

/* sys6 get cpu time */
void getCpuTime() {
    /* Initialize current time */
    cpu_t TOD_c;
    STCK(TOD_c);
    
    /* Calculate total CPU time: stored time + time spent in current quantum */
    TOD_c = (TOD_c - TOD_start) + currentProcess->p_time;
    currentProcess->p_s.s_v0 = TOD_c;

    /* Return to the process */
    loadNextState(currentProcess->p_s);;
}


/* sys7 wait for clock */
void waitClock() {
    /* Block the process */
    passeren(&deviceSemaphores[DEVICE_COUNT]);
}

/* sys8 get support data */
void getSupportData() {
    currentProcess->p_s.s_v0 = (int) currentProcess->p_supportStruct;
    loadNextState(currentProcess->p_s);;
}

void copyState(state_t *source, state_t *dest) {
    dest->s_entryHI = source->s_entryHI;
    dest->s_cause = source->s_cause;
    dest->s_status = source->s_status;
    dest->s_pc = source->s_pc;
    
    int i;
    for (i = 0; i < STATEREGNUM; i++) {
        dest->s_reg[i] = source->s_reg[i];
    }
}

void passUpOrDie(unsigned int passUpCode)
{
	if ((currentProcess->p_supportStruct) != NULL) {
        
		state_PTR exceptStatePtr = (state_PTR) BIOSDATAPAGE;
        copyState(&currentProcess->p_supportStruct->sup_exceptState[exceptionType], exceptionState);

		LDCXT(currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_stackPtr, 
              currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_status, 
              currentProcess->p_supportStruct->sup_exceptContext[passUpCode].c_pc);
	}
	else {
		terminateProcess(currentProcess); 
		scheduler();
    }
}

void tlbExceptionHandler() {
	passUpOrDie(PGFAULTEXCEPT);
}

void programTrapHandler() {
	passUpOrDie(GENERALEXCEPT);
}

