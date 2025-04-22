#include "../h/sysSupport.h"

/*****************************************************************************
 *  sysSupport.c
 *
 *  This file is the implementation for handling passed-up exceptions and
 *  SYSCALLs from SYS9 to SYS13 for user processes (U-procs).
 *
 *  Description:
 *  This module defines the Support Level handlers for system calls and
 *  exceptions (SYS9 – SYS13). It extends the “Pass Up or Die” model in that
 *  user-mode exceptions are handled here if a support structure exists;
 *  otherwise, the U-proc is terminated.
 *
 *  Implementation:
 *  1. supGeneralExceptionHandler – Dispatches based on Cause.ExcCode:
 *     - SYSCALL_EXCEPTION: invokes supSyscallHandler
 *     - All others: handled by supProgramTrapHandler (treated as fatal)
 *
 *  2. supProgramTrapHandler – Invoked for illegal ops or faults
 *     Terminates the current U-proc via terminateUProc()
 * 
 *  3. supSyscallHandler – Handles system calls (SYS9–SYS13)
 *
 *  System Calls:
 *  - SYS9: terminateUProc – Frees swap resources and terminates the U-proc
 *  - SYS10: getTod – Returns time-of-day clock value (via STCK)
 *  - SYS11: writeToPrinter – Sends string to assigned printer device
 *  - SYS12: writeToTerminal – Sends string to terminal output
 *  - SYS13: readFromTerminal – Reads a line from terminal input (until EOL)
 *
 *  Each syscall validates user input, manages device semaphores, and uses
 *  LDST to resume user execution upon completion or failure.
 *
 *  Written by: Hieu Tran and Khoa Ho
 *  April 2025
 ****************************************************************************/

/* Syscall handler functions */
HIDDEN void terminateUProc(int* sem);
HIDDEN void getTod(state_t *excState);
HIDDEN void writeToPrinter(state_t *excState, int asid);
HIDDEN void writeToTerminal(state_t *excState, int asid);
HIDDEN void readFromTerminal(state_t *excState, int asid);

/*****************************************************************************
 *  Function: supGeneralExceptionHandler
 *
 *  Handles general exceptions and system calls by checking the cause of the 
 *  exception and dispatches to the appropriate handler 
 *  (supSyscallHandler or supProgramTrapHandler). 
 */
void supGeneralExceptionHandler() {
    /* get support structure */
    support_t *supportPtr = (support_t*) SYSCALL(GETSUPPORT, 0, 0, 0);
    state_PTR savedExceptionState = &(supportPtr->sup_exceptState[GENERALEXCEPT]);

    /* increment PC to next instruction */
    savedExceptionState->s_pc += WORDLEN;

    /* get exception type */
    int cause = CAUSE_GET_EXCCODE(savedExceptionState->s_cause);

    /* check if exception is syscall */
    if (cause == SYSCALL_EXCEPTION) {
        supSyscallHandler(supportPtr);
    } else {
        supProgramTrapHandler();
    }
}

/*****************************************************************************
 *  Function: supSyscallHandler
 *
 *  Handles system calls (SYS9–SYS13) by checking the syscall code and 
 *  dispatching to the appropriate handler for each syscall.
 * 
 *  Parameters:
 *  supportPtr: pointer to the support structure of the current U-proc
 */
void supSyscallHandler(support_t *supportPtr) {
    state_t *excState = &(supportPtr->sup_exceptState[GENERALEXCEPT]);

    int asid = supportPtr->sup_asid;

    /* get syscall code */
    int syscallCode = excState->s_a0;
    switch (syscallCode) {
        case TERMINATE: {
            terminateUProc(NULL);  /* SYS9 */
            break;
        }
        case GETTOD: {
            getTod(excState);  /* SYS10 */
            break;
        }
        case WRITEPRINTER: {
            writeToPrinter(excState, asid);  /* SYS11 */
            break;
        }
        case WRITETERMINAL: {
            writeToTerminal(excState, asid);  /* SYS12 */
            break;
        }
        case READTERMINAL: {
            readFromTerminal(excState, asid);  /* SYS13 */
            break;
        }
        default: {
            supProgramTrapHandler();  /* unknown syscall - terminate process */
        }
    }
}
/*****************************************************************************
 *  Function: supProgramTrapHandler
 *
 *  Handles program traps (illegal operations or faults) by terminating the 
 *  current U-proc and releasing any held resources.
 */
void supProgramTrapHandler() {
    terminateUProc(NULL);
}

/*****************************************************************************
 *  Function: terminateUProc (SYS9)
 *
 *  Terminates the current U-proc by releasing resources and terminating the 
 *  process. If a semaphore is provided, it releases the mutex on the swap 
 *  pool semaphore.
 * 
 *  Parameters:
 *  sem: pointer to the semaphore to be released 
 */
void terminateUProc(int* sem) {
    /* get current process */
    int asid = currentProcess->p_supportStruct->sup_asid;

    /* clear swap pool entry */
    markAllFramesFree(asid);

    /* check if process holds mutex on swap pool semaphore*/
    if (sem != NULL) {
        /* release mutex */
        mutex(OFF, sem);
    }
    /* release mutex on master semaphore */
    mutex(OFF, (int *) &masterSemaphore);

    /* terminate process */
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

/*****************************************************************************
 *  Function: getTod (SYS10)
 *
 *  Gets the current time-of-day clock value and stores it in the v0 register.
 * 
 *  Parameters:
 *  excState: pointer to the exception state structure
 */
void getTod(state_t *excState) {
    /* get the number microseconds since the system booted */
    cpu_t tod;
    STCK(tod);

    /* store time in v0 register */
    excState->s_v0 = tod;
    LDST(excState);
}

/*****************************************************************************
 *  Function: writeToPrinter (SYS11)
 *
 *  Sends a string to the assigned printer device. It validates the input 
 *  string and length, manages the device semaphore, and handles errors.
 * 
 *  Parameters:
 *  excState: pointer to the exception state structure
 *  asid: the assigned process ID
 */
void writeToPrinter(state_t *excState, int asid) {
    /* variables for busy printer check */
    int i = 0; /* count of chars transmitted */
    int error = 0;
    int status, statusCode;

    /* get virtual address of first char of transmitted string */
    char *virtAddr = (char *) excState->s_a1;

    /* get length of transmitted string */
    int length = (int) excState->s_a2;

    /* check if address is in user space and length is valid */
    if ((int) virtAddr < KUSEG || length < 0 || length > MAXSTRLEN) {
        supProgramTrapHandler();
    }
    /* get device register address */
    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;

    /* get printer device number */
    int printNo = asid - 1; /*asid maps to device number, starts from 1*/ 
    /* get printer device semaphore */
    int printSem = ((PRNTINT - DISKINT) * DEVPERINT) + printNo;

    /* get mutex for printer device */
    mutex(ON, &devSemaphore[printSem]);

    /* check if printer device is busy */
    while (!error && i < length) {
        /* loop until all chars are transmitted or error occurs */
        devrega->devreg[printSem].d_data0 = *virtAddr;
        toggleInterrupts(OFF);
        devrega->devreg[printSem].d_command = PRINTCHR;
        status = SYSCALL(WAITFORIO, PRNTINT, printNo, 0);
        toggleInterrupts(ON);

        /* check status code */
        statusCode = status & STATUS_MASK;
        if (statusCode == READY) {
            i++;
            virtAddr++;
        } else {
            /* error occurred*/
            error = 1;
        }
    }
    if (!error) {
        /* save number of chars transmitted in v0 register */
        excState->s_v0 = i;
    } else {
        /* save negative of device's status code in v0 register */
        excState->s_v0 = -statusCode;
    }
    /* release mutex */
    mutex(OFF, &devSemaphore[printSem]);
    LDST(excState);
}

/*****************************************************************************
 *  Function: writeToTerminal (SYS12)
 *
 *  Sends a string to the assigned terminal device. It validates the input 
 *  string and length, manages the device semaphore, and handles errors.
 * 
 *  Parameters:
 *  excState: pointer to the exception state structure
 *  asid: the assigned process ID
 */
void writeToTerminal(state_t *excState, int asid) {
    /* variables for busy terminal check */
    int i = 0;
    int error = 0;
    int status, statusCode;

    /* get virtual address of first char of transmitted string */
    char *virtAddr = (char *) excState->s_a1; 

    /* get length of transmitted string */
    int length = (int) excState->s_a2;

    /* check if address is in user space and length is valid */
    if ((int) virtAddr < KUSEG || length < 0 || length > MAXSTRLEN) {
        supProgramTrapHandler();
    }

    /* check if address is in user space and length is valid */
    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;  

    /* get device register address */
    int termNo = asid - 1; /*asid maps to device number, starts from 1*/ 
    /* get terminal device number */
    int termSem = ((TERMINT - DISKINT) * DEVPERINT) + termNo;

    /* get mutex for terminal device */
    mutex(1, &devSemaphore[termSem + DEVPERINT]);

    /* check if terminal device is busy */
    while (!error && i < length) {
        /* loop until all chars are transmitted or error occurs */
        toggleInterrupts(0);
        devrega->devreg[termSem].t_transm_command = (*virtAddr << BITSHIFT_8) | TRANSTATUS;
        status = SYSCALL(WAITFORIO, TERMINT, termNo, 0);
        toggleInterrupts(1);

        /* check status code */
        statusCode = status & STATUS_MASK;
        if (statusCode == CHAR_TRANSMITTED) {
            i++;
            virtAddr++;
        } else {
            /* error occurred */
            error = 1;
        }
    }
    if (!error) {
        /* save number of chars transmitted in v0 register */
        excState->s_v0 = i;
    } else {
        /* save negative of device's status code in v0 register */
        excState->s_v0 = -statusCode;
    }
    /* release mutex */
    mutex(0, (int *) &devSemaphore[termSem + DEVPERINT]);
    LDST(excState);
}

/*****************************************************************************
 *  Function: readFromTerminal (SYS13)
 *
 *  Reads a line from the assigned terminal device until EOL. It validates 
 *  the input address, manages the device semaphore, and handles errors.
 * 
 *  Parameters:
 *  excState: pointer to the exception state structure
 *  asid: the assigned process ID
 */
void readFromTerminal(state_t *excState, int asid) {
    /* variables for busy terminal check */
    int i = 0;
    int error = 0; 
    int done = 0;
    int status, statusCode;

    /* get virtual address of first char of received string */
    char *virtAddr = (char *) excState->s_a1;

    /* get length of received string */
    if ((int) virtAddr < KUSEG) {
        supProgramTrapHandler();
    }
    /* check if address is in user space and length is valid */
    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;

    /* get device register address */
    int termNo = asid - 1; /*asid maps to device number, starts from 1*/ 
    /* get terminal device number */
    int termSem = ((TERMINT - DISKINT) * DEVPERINT) + termNo;

    /* get mutex for terminal device */
    mutex(ON, &devSemaphore[termSem]);

    /* check if terminal device is busy */
    while (!error && !done) {
        /* loop until all chars are received or error occurs */
        toggleInterrupts(OFF);
        devrega->devreg[termSem].t_recv_command = TRANSTATUS;
        status = SYSCALL(WAITFORIO, TERMINT, termNo, 1);
        toggleInterrupts(ON);

        /* check status code */
        statusCode = status & STATUS_MASK;
        if (statusCode == CHAR_RECEIVED) {
            /* extract the character from the status (upper half) */
            *virtAddr = (status >> BITSHIFT_8) & BITMASK_8; 
            virtAddr++;
            i++;

            if (status >> BITSHIFT_8 == EOL) {
                done = 1;
            }
        } else {
            /* error occurred */
            error = 1;
        }
    }
    /* release mutex */
    mutex(OFF, &devSemaphore[termSem]);

    if (!error) {
        /* save number of chars received in v0 register */
        excState->s_v0 = i;
    } else {
        /* save negative of device's status code in v0 register */
        excState->s_v0 = -statusCode;
    }
    LDST(excState);
}
