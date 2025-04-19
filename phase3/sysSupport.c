#include "../h/sysSupport.h"

void debug1 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug2 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug3 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug4 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug5 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug6 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug7 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug12 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug13 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}
void debug14 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}
void debug15 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}
void debug16 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}


void supGeneralExceptionHandler() {
    /* get support structure */
    support_t *supportPtr = (support_t*) SYSCALL(GETSUPPORT, 0, 0, 0);

    /* get exception type */
    int cause = CAUSE_GET_EXCCODE(supportPtr->sup_exceptState[GENERALEXCEPT].s_cause);

    /* check if exception is syscall */
    if (cause == SYSCALL_EXCEPTION) {
        supSyscallHandler(supportPtr);
    } else {
        supProgramTrapHandler();
    }
}

void supSyscallHandler(support_t *supportPtr) {
    state_t *excState = &(supportPtr->sup_exceptState[GENERALEXCEPT]);

    int asid = supportPtr->sup_asid;

    /* increment pc to next instruction */
    excState->s_pc += WORDLEN;

    /* get syscall code */
    int syscallCode = excState->s_a0;
    switch (syscallCode) {
        case TERMINATE: {
            debug12(0, 0, 0, 0);
            terminateUProc(NULL);  /* SYS9 */
            debug1(0, 0, 0, 0);
            break;
        }
        case GETTOD: {
            debug13(0, 0, 0, 0);
            getTod(excState);  /* SYS10 */
            debug2(0, 0, 0, 0);
            break;
        }
        case WRITEPRINTER: {
            debug14(0, 0, 0, 0);

            writeToPrinter(excState, asid);  /* SYS11 */
            debug3(0, 0, 0, 0);
            break;
        }
        case WRITETERMINAL: {
            debug15(0, 0, 0, 0);

            writeToTerminal(excState, asid);  /* SYS12 */
            debug4(0, 0, 0, 0);
            break;
        }
        case READTERMINAL: {
            debug16(0, 0, 0, 0);
            readFromTerminal(excState, asid);  /* SYS13 */
            debug5(0, 0, 0, 0);
            break;
        }
        default: {
            debug7(0, 0, 0, 0);		
            supProgramTrapHandler();  /* Unknown syscall - terminate process */
            debug6(0, 0, 0, 0);
        }
    }
    /* load state from support struct */
/*     LDST(excState);  */
}
        

void supProgramTrapHandler() {
    terminateUProc(NULL);
}

/*sys9*/
void terminateUProc(int* sem) {
    /* get asid of current process */
    int asid = currentProcess->p_supportStruct->sup_asid;

    /* clear swap pool entry */
    int i = 0;
    for (i = 0; i < POOLSIZE; i++) {
        if (swapPool[i].swap_asid == asid) {
            swapPool[i].swap_asid = FREEFRAME;
        }
    }
    /* check if process holds mutex on swap pool semaphore*/
    if (sem != NULL) {
        /* release mutex */
        mutex(0, sem);
    }

    /* release master semaphore */
    mutex(0, (int *) &masterSemaphore);

    /* terminate process */
    SYSCALL(TERMPROCESS, 0, 0, 0);
}

/*sys10*/
void getTod(state_t *excState) {
    /* get current time */
    cpu_t tod;
    STCK(tod);

    /* store time in v0 register */
    excState->s_v0 = tod;
    LDST(excState);
}

/*sys11*/
void writeToPrinter(state_t *excState, int asid) {
    /* get virtual address of first char of transmitted string */
    char *virtAddr = (char *) excState->s_a1;

    /* get length of transmitted string */
    int length = (int) excState->s_a2;

    if ((int) virtAddr < KUSEG || length < 0 || length > MAXSTRLEN) {
        supProgramTrapHandler();
    }

    /* get device register address */
    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;

    /* get printer device number */
    int printNo = asid - 1; /*asid maps to devnum, starts from 1*/ 
    
    /* get printer device semaphore */
    int printSem = ((PRNTINT - DISKINT) * DEVPERINT) + printNo;

    /* get mutex for printer device */
    mutex(1, &devSemaphore[printSem]);

    int error = 0;
    unsigned int status, statusCode;
    int i = 0;
    while (!error && i < length) {
        devrega->devreg[printSem].d_data0 = *virtAddr;
        toggleInterrupts(0);
        devrega->devreg[printSem].d_command = PRINTCHR;
        status = SYSCALL(WAITFORIO, PRNTINT, printNo, 0);
        toggleInterrupts(1);

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
    mutex(0, &devSemaphore[printSem]);
    LDST(excState);
}

void writeToTerminal(state_t *excState, int asid) {
    char *virtAddr = (char *) excState->s_a1;
    int length = (int) excState->s_a2;

    if ((int) virtAddr < KUSEG || length < 0 || length > MAXSTRLEN) {
        supProgramTrapHandler();
    }

    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;
    int termNo = asid - 1;
    int termSem = ((TERMINT - DISKINT) * DEVPERINT) + termNo;

    /* get mutex for terminal device */
    mutex(1, &devSemaphore[termSem + DEVPERINT]);

    int error = 0;
    unsigned int status, statusCode;
    int i = 0;
    while (!error && i < length) {
        toggleInterrupts(0);
        devrega->devreg[termSem].t_transm_command = (*virtAddr << BITSHIFT_8) | TRANSTATUS;
        status = SYSCALL(WAITFORIO, TERMINT, termNo, 0);
        toggleInterrupts(1);

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

void readFromTerminal(state_t *excState, int asid) {
    char *virtAddr = (char *) excState->s_a1;

    if ((int) virtAddr < KUSEG) {
        supProgramTrapHandler();
    }

    devregarea_t *devrega = (devregarea_t *) RAMBASEADDR;
    int termNo = asid - 1;
    int termSem = ((TERMINT - DISKINT) * DEVPERINT) + termNo;

    /* get mutex for terminal device */
    mutex(1, &devSemaphore[termSem]);

    int error = 0;
    int status, statusCode;
    int i = 0;
    int done = 0;
    while (!error && !done) {
        toggleInterrupts(0);
        devrega->devreg[termSem].t_recv_command = TRANSTATUS;
        status = SYSCALL(WAITFORIO, TERMINT, termNo, 1);
        toggleInterrupts(1);

        statusCode = status & STATUS_MASK;
        if (statusCode == CHAR_RECEIVED) {
            *virtAddr = (status >> BITSHIFT_8) & BITMASK_8; /* Extract the character from the status (upper half) */
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
    mutex(0, &devSemaphore[termSem]);

    if (!error) {
        /* save number of chars received in v0 register */
        excState->s_v0 = i;
    } else {
        /* save negative of device's status code in v0 register */
        excState->s_v0 = -statusCode;
    }
    LDST(excState);
}



