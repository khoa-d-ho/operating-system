#include "../h/scheduler.h"

/****************************************************************************
 * scheduler.c
 * 
 * This file contains the implementation of the scheduler for the operating
 * system. The scheduler is a simple preemptive round-robin scheduling 
 * algorithm with a time slice value of 5 milliseconds.
 * 
 * Written by Khoa Ho & Hieu Tran
 * February 2025
 ****************************************************************************/

/*****************************************************************************
 * Function: scheduler
 * 
 * This function implements a simple round-robin scheduler. It is called by
 * the operating system to switch between processes.
 * 
 * The scheduler uses a time slice of 5 milliseconds. If a process does not
 * complete within this time, it is preempted and the next process in the
 * ready queue is scheduled.
 */
void scheduler() {
    if(!emptyProcQ(readyQueue)) {
        /* Initializes current process */
        currentProcess = removeProcQ(&readyQueue);
        setTIMER(QUANTUM);
        STCK(TOD_start);
        loadNextState(currentProcess->p_s);
    }
    else {
        if(processCount == 0){
            /* If no more processes */
            HALT();
        }
        
        else if(processCount > 0 && softBlockCount > 0) {
            /* If blocked processes */
            setTIMER(XLVALUE);
            setSTATUS(ALLOFF | IECON | IMON | TEBITON);
            WAIT();
        }
        else if(processCount > 0 && softBlockCount <= 0){
            /* If deadlocked */
            PANIC();
        }
    }
}

void loadNextState(state_t state) {
    LDST(&state);
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
