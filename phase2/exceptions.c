#include "../h/exceptions.h"

HIDDEN void storeState(state_PTR oldState) {
    oldState->s_pc += WORDLEN;
    copyState(oldState, &(currentProcess->p_s));
}

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
		passUpOrDie(GENERALEXCEPT);
	}
}

void syscallHandler() {
    state_t *excState = (state_t *) BIOSDATAPAGE;
    int syscallCode = excState->s_a0;
    
    if (currentProcess->p_s.s_status & KUPON && syscallCode <= 8) {
        programTrapHandler();
    }
    
    switch (syscallCode) {
        case CREATEPROCESS: {
            createProcess(); 
        }
        case TERMPROCESS: {
            terminateProcess(currentProcess);
        } 
        case PASSEREN: {
            passeren();
        }
        case VERHOGEN: {
            verhogen(); 
        }
        case WAITFORIO: {
            waitIO(); 
        }
        case GETCPUTIME: {
            getCPUTime();
        }
        case WAITFORCLOCK: {
            waitClock(); 
        }
        case GETSUPPORT: {
            getSupportData();
        }
        default: {			
            passUpOrDie(GENERALEXCEPT); 
        }
    }
}

void createProcess() {
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    pcb_PTR newProc = allocPcb();
    
    if (newProc == NULL) {
        oldState->s_v0 = -1;
    } else {
        /* Set the new process state to state stored in a1 */
        state_t *toCheck = (state_t*) oldState->s_a1;
        copyState(toCheck, &newProc->p_s);

        /* Copying over the support struct ... */
        support_t *supportData = (support_t*) oldState->s_a2;

        if (supportData != NULL) {
            newProc->p_supportStruct = supportData;
        }

        /* Incrementing process count, inserting on ready queue, making a child of current process, and returning control */
        processCount++;
        insertProcQ(&readyQueue, newProc);
        insertChild(currentProcess, newProc);
        oldState->s_v0 = 0;
    }
    storeState(oldState);
    loadNextState(currentProcess->p_s);
}

/* sys2 terminate process */
void terminateProcess(pcb_PTR current) {
	while (emptyChild(current) == FALSE) {
		terminateProcess(removeChild(current));
	}

	if ((current->p_semAdd) != NULL) {
		if ((current->p_semAdd >= &deviceSemaphores[0]) && (current->p_semAdd <= &deviceSemaphores[CLOCK])) {
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
		pcb_PTR p = outProcQ(&readyQueue, current);
        if (p != mkEmptyProcQ()) {
			processCount--;
		}
	}

	if (current == currentProcess) {
		if (!emptyChild(current))  {
            outChild(currentProcess);
        }
		currentProcess = mkEmptyProcQ();
		processCount--;  
		scheduler();
	}
    freePcb(current);
}

/* sys3 passeren */
void passeren() { 
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    cpu_t TOD_current;
    STCK(TOD_current);
    int *semAddress = (int *) oldState->s_a1;

    (*semAddress)--;

    storeState(oldState);
    if (*semAddress < 0) {
        insertBlocked(semAddress, currentProcess);
        currentProcess->p_time += (TOD_current - TOD_start);
        currentProcess = NULL;
        scheduler();
    }
    loadNextState(currentProcess->p_s);
}

/* sys4 verhogen */
void verhogen() { 
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    int *semAddress = (int *) oldState->s_a1;
    (*semAddress)++;
    pcb_PTR p;
    if (*semAddress <= 0) {
        p = removeBlocked(semAddress);
        if (p != NULL) {
            insertProcQ(&readyQueue, p);
        }
    }
    storeState(oldState);
    loadNextState(currentProcess->p_s);
}

/* sys5 wait for io */
void waitIO() {
    cpu_t TOD_stop;
    STCK(TOD_stop);
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    
    /* Get line, device number, and possible terminal read operations */
    int line = oldState->s_a1;
    int dev = oldState->s_a2;
    int term_flag = oldState->s_a3;

    /* Calculate semaphore index */
    int semIndex;
    if (term_flag && line == TERMINT) {
        /* Terminal device*/
        semIndex = dev + (line - DISKINT) * DEVPERINT + DEVPERINT; 
    } else {
        /* Non-terminal device */
        semIndex = dev + (line - DISKINT) * DEVPERINT;             
    }

    deviceSemaphores[semIndex]--;
    if(deviceSemaphores[semIndex] < 0) {
        storeState(oldState);
        currentProcess->p_time += (TOD_stop - TOD_start);
        insertBlocked(&(deviceSemaphores[semIndex]), currentProcess);
        
        currentProcess = mkEmptyProcQ();
        softBlockCount++;
        scheduler();
    }
    loadNextState(currentProcess->p_s);
}

/* sys6 get cpu time */
void getCPUTime() {
    cpu_t TOD_current;
    STCK(TOD_current);    
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    oldState->s_v0 = currentProcess->p_time + TOD_current - TOD_start;

    storeState(oldState);
    loadNextState(currentProcess->p_s);
}

/* sys7 wait for clock */
void waitClock() {
    int TOD_stop;
	STCK(TOD_stop);
	state_PTR oldState = (state_PTR) BIOSDATAPAGE;

    (deviceSemaphores[DEVICE_COUNT-1])--;

    if (deviceSemaphores[DEVICE_COUNT-1] >= 0) {
        loadNextState(currentProcess->p_s);
    }
	
    softBlockCount++;
    storeState(oldState);
    currentProcess->p_time += (TOD_stop - TOD_start);
    insertBlocked(&(deviceSemaphores[DEVICE_COUNT-1]), currentProcess);
    currentProcess = NULL;

	scheduler();
}

void getSupportData() {
    state_PTR oldState = (state_PTR) BIOSDATAPAGE;
    oldState->s_v0 = (int) (currentProcess->p_supportStruct);    
    storeState(oldState);
    loadNextState(currentProcess->p_s);
    scheduler();
}

void passUpOrDie(int passUpCode)
{
	if ((currentProcess->p_supportStruct) != NULL) {
		state_PTR exceptStatePtr = (state_PTR) BIOSDATAPAGE;
        copyState(exceptStatePtr, &currentProcess->p_supportStruct->sup_exceptState[passUpCode]);

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
