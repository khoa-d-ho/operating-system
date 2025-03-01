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

#include "../h/scheduler.h"
#include "../h/initial.h"

/*****************************************************************************
 * Function: scheduler
 * 
 * This function implements a simple round-robin scheduler. It is called by
 * the operating system to switch between processes.
 * 
 * The scheduler uses a time slice of 5 milliseconds. If a process does not
 * complete within this time, it is preempted and the next process in the
 * ready queue is scheduled.
 ****************************************************************************/
void scheduler() {
    /* Check if the ready queue is empty */
    if (emptyProcQ(readyQueue)) {
        /* If there are no processes in the system, halt */
        if (processCount == 0) {
            HALT();
        }
        /* If there are processes but all are blocked, wait */
        else if (softBlockCount > 0) {
            /* Set status register to enable interrupts and disable PLT */
            setSTATUS(IECON | IMON | TEBITON);
            LDIT(MAXINT);
            WAIT();
        }
        /* Deadlock detected, invoke PANIC */
        else {
            PANIC();
        }
    }

    /* Remove the head process from the ready queue */
    pcb_PTR nextProcess = removeProcQ(&readyQueue);
    currentProcess = nextProcess;

    /* Load 5 miliseconds on the PLT */
    LDIT(5000);

    /* Load the processor state of the current process */
    LDST(&(currentProcess->p_s));
}