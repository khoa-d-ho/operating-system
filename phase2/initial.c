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
cpu_t startTOD;             /* start time of day */

extern void test();
/* extern void uTLB_RefillHandler(); */

void main() {

    /* 2. pass up vectors */

    passupvector_t *passupvector = (memaddr) PASSUPVECTOR;
    passupvector->tlb_refll_handler = (memaddr) uTLB_RefillHandler; 
    passupvector->tlb_refll_stackPtr = (memaddr) (RAMTOP);
    passupvector->exception_handler = (memaddr) exceptionHandler;
    passupvector->exception_stackPtr = (memaddr) (RAMTOP);

    /* 3. init level 2 dsa */
    initPcbs();
    initASL();
    
    /* 4. init nucleus maintained global vars */
    processCount = 0;
    softBlockCount = 0;
    readyQueue = mkEmptyProcQ();
    currentProcess = NULL;

    /* init device semaphores */
    int i = 0;
    for (i = 0; i < DEVICE_COUNT; i++) {
        deviceSemaphores[i] = 0;
    }

    /* 5. interval timer */
    LDIT(100000);  /* assign later !!! 100ms */

    /* Create and initialize first process */
    pcb_PTR firstProcess = allocPcb();
    if (firstProcess == NULL) {
        PANIC();  
    }

    /* Initialize process state */

    /* Set up first process */
    firstProcess->p_s.s_pc = (memaddr)test;    /* Set PC to test */
    firstProcess->p_s.s_t9 = (memaddr)test;  /* Set t9 to test */
    firstProcess->p_s.s_status = ALLOFF | IECON | IMON | TEBITON;  /* Set status register */
    firstProcess->p_s.s_sp = RAMTOP - FRAMESIZE;  /* Set stack pointer to top of RAM */
    
    /* Add to ready queue and update process count */
    insertProcQ(&readyQueue, firstProcess);
    processCount++;

    /* Read Start time */
    STCK(startTOD);

    /* Call scheduler */
    scheduler();
}