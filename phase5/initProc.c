#include "../h/initProc.h"

/****************************************************************************
 * initProc.c
 * 
 * This file contains the implementation of the process initialization module.
 * It initializes the user processes and their support structures, including
 * the page table entries and exception handlers.
 * 
 * Written by Khoa Ho & Hieu Tran
 * April 2025
 ****************************************************************************/

/* Global & local variables */
int devSemaphore[DEVICE_COUNT-1]; 
int masterSemaphore; 
HIDDEN support_t supStructs[UPROCMAX+1];

/* Helper functions */
HIDDEN void initUProc(int id);
HIDDEN void configSupStruct(int id);

/* Main test function - initializes and controls user processes */
void test() {
    /* Initialize device semaphores */
    int i;
    for (i = 0; i < (DEVICE_COUNT-1); i++) {
        devSemaphore[i] = 1;
    }

    /* Initialize swap structures for VM */
    initSwapStructs();

    /* Create user processes */
    int id;
    for (id = 1; id <= UPROCMAX; id++) {
        initUProc(id);
    }
    
    /* Wait for all user processes to complete */
    masterSemaphore = 0;
    for (i = 0; i < UPROCMAX; i++) {
        SYSCALL(PASSEREN, (int) &masterSemaphore, 0, 0);
    }

    /* Terminate the init process */
    SYSCALL(TERMPROCESS, 0, 0, 0);
} 

/****************************************************************************
 * Function: initUProc
 * 
 * This function initializes a user process with the given ID. It sets up the
 * initial state of the process, including the program counter, stack pointer,
 * and status register. It also configures the support structures for the user
 * process and creates the process using a system call.
 * 
 * Parameters:
 *   id - The ID of the user process to be initialized.
 */
HIDDEN void initUProc(int id) {
    /* Configure initial state for the user process */
    state_t newState;
    newState.s_pc = (memaddr) TEXTAREAADDR;
    newState.s_t9 = (memaddr) TEXTAREAADDR;
    newState.s_sp = (memaddr) STACKPAGEADDR;
    newState.s_status = ALLOFF | TEBITON | IMON | IEPON | UMON;
    newState.s_entryHI = KUSEG | (id << ASIDSHIFT) | ALLOFF;

    /* Configure support structures for the user process */
    configSupStruct(id);

    /* Create the process using syscall */
    int status = SYSCALL(CREATEPROCESS, (int) &(newState), (int) &(supStructs[id]), 0);
        
    if (status != OK) {
        /* Terminate if creation failed */
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }
}

/****************************************************************************
 * Function: configSupStruct
 * 
 * This function configures the support structure for a user process with the
 * given ID. It sets up the ASID, exception contexts, and page table entries.
 * 
 * Parameters:
 *   id - The ID of the user process to be configured.
 */
HIDDEN void configSupStruct(int id) {
    /* Basic ASID assignment */
    supStructs[id].sup_asid = id;
    
    /* Configure context for general exceptions */
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) supGeneralExceptionHandler;
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr) &(supStructs[id].sup_stackGen[TOPSTACK]);
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IEPON | IMON | TEBITON;
    
    /* Configure context for TLB exceptions */
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) supTlbExceptionHandler;
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr) &(supStructs[id].sup_stackTLB[TOPSTACK]);
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IEPON | IMON | TEBITON;
    
    /* Initialize page table entries */
    int pg;
    for(pg = 0; pg < MAXPAGES; pg++) {
        supStructs[id].sup_privatePgTbl[pg].entryHI = ALLOFF | ((UPROCSTART + pg) << VPNSHIFT) | (id << ASIDSHIFT);
        supStructs[id].sup_privatePgTbl[pg].entryLO = ALLOFF | DIRTYON;
    }

    /* Configure stack page */
    supStructs[id].sup_privatePgTbl[MAXPAGES-1].entryHI = ALLOFF | (PAGESTACK << VPNSHIFT) | (id << ASIDSHIFT);
}
