/****************************************************************************
 * initial.c
 *
 * This module initializes the nucleus of the operating system.
 *
 * Written by Khoa Ho & Hieu Tran
 * February 2025
 ****************************************************************************/

#include "/usr/include/umps3/umps/libumps.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"

/* 1. declare level 3 global vars */
int processCount;            
int softBlockCount;                 
pcb_PTR readyQueue;               
pcb_PTR currentProcess;          
int deviceSemaphores[DEVICE_COUNT];  
cpu_t TOD_start;             /* start time of day */

extern void test();
extern void uTLB_RefillHandler();

void main() {
    /* 2. pass up vectors */
    passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR;
    passupvector->tlb_refll_handler = (memaddr) uTLB_RefillHandler; 
    passupvector->tlb_refll_stackPtr = (memaddr) (RAMTOP);
    passupvector->exception_handler = (memaddr) exceptionHandler; 
    passupvector->exception_stackPtr = (memaddr) (RAMTOP);

    /* Store current time of day */
    STCK(TOD_start);

    /* Get RAM top from device registers (if needed) */
    devregarea_t *devrega = (devregarea_t*) RAMBASEADDR;
    memaddr ramtop = (devrega->rambase) + (devrega->ramsize);

    /* 3. init level 2 dsa */
    initPcbs();
    initASL();
    
    /* 4. init nucleus maintained global vars */
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;

    /* init device semaphores */
    int i;
    for (i = 0; i < DEVICE_COUNT; i++) {
        deviceSemaphores[i] = 0;
    }

    /* 5. interval timer */
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

    /* Call scheduler */
    scheduler();
}