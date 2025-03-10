#include "../h/initial.h"

/****************************************************************************
 * initial.c
 *
 * This module initializes the nucleus of the operating system. 
 * 
 * This module ensures that the first process is created and added to the ready
 * queue. It also sets up the pass up vectors for TLB refill and exception
 * handling. The first process is set up with the test program as its code and
 * the nucleus stack as its stack. The scheduler is then called to start the
 * operating system. 
 * 
 * Written by Khoa Ho & Hieu Tran
 * February 2025
 ****************************************************************************/

void main() {
    /* Populate pass up vectors */
    passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR;
    passupvector->tlb_refll_handler = (memaddr) uTLB_RefillHandler; 
    passupvector->tlb_refll_stackPtr = (memaddr) NUCLEUSSTACKTOP;
    passupvector->exception_handler = (memaddr) exceptionHandler; 
    passupvector->exception_stackPtr = (memaddr) NUCLEUSSTACKTOP;

    /* Get RAM top from device registers */
    devregarea_t *devrega = (devregarea_t*) RAMBASEADDR;
    memaddr ramtop = (devrega->rambase) + (devrega->ramsize);

    /* Initialize level 2 data structures */
    initPcbs();
    initASL();
    
    /* Initialize nucleus maintained variables */
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;

    /* Initialize device semaphores */
    int i;
    for (i = 0; i < DEVICE_COUNT; i++) {
        deviceSemaphores[i] = 0;
    }

    LDIT(CLOCKINTERVAL);  /* 100ms */

    /* Create and initialize first process */
    pcb_PTR firstProcess = allocPcb();
    if (firstProcess == NULL) {
        PANIC();  
    }

    /* Set up first process */
    firstProcess->p_s.s_pc = (memaddr)test;    /* Set PC to test */
    firstProcess->p_s.s_t9 = (memaddr)test;    /* Set t9 to test */
    firstProcess->p_s.s_status = ALLOFF | IEPON | IMON | TEBITON;  /* Set status register */
    firstProcess->p_s.s_sp = ramtop - FRAMESIZE;  /* Set stack pointer to top of RAM */
    firstProcess->p_time = 0;
    firstProcess->p_semAdd = NULL;
    firstProcess->p_supportStruct = NULL;
    firstProcess->p_prnt = NULL;
    
    /* Add to ready queue and update process count */
    insertProcQ(&readyQueue, firstProcess);
    processCount++;

    scheduler();
}