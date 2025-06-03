#include "../h/vmSupport.h"

/****************************************************************************
 * vmSupport.c
 * 
 * This file contains the implementation of the virtual memory support functions
 * for the operating system. It includes functions for handling TLB exceptions
 * (the Pager), managing the swap pool, and performing I/O operations on the 
 * flash device.
 * 
 * Written by Khoa Ho & Hieu Tran
 * April 2025
 *****************************************************************************/

/* Local variables */
HIDDEN swap_t swapPool[POOLSIZE]; /* swap pool table */
HIDDEN int swapPoolSem; /* semaphore for swap pool */

/******************************************************************************
 * Function: initSwapStructs
 * 
 * This function initializes the swap pool table entries and the semaphore for
 * the swap pool. It sets all entries in the swap pool to FREEFRAME (-1) and sets
 * the semaphore to 1 (mutex).
 */
void initSwapStructs() {
    int i;
    
    /* initialize swap pool table entries */
    for (i = 0; i < POOLSIZE; i++) {
        swapPool[i].swap_asid = FREEFRAME;
    }

    /* initialize semaphore to 1 (mutex) */
    swapPoolSem = 1;
}

/******************************************************************************
 * Function: mutex
 * 
 * This function handles mutual exclusion for the swap pool. It takes a pointer
 * to the semaphore and either passes it to the P operation or the V operation
 * depending on the value of the 'on' parameter. If 'on' (1), it performs
 * the P operation, otherwise it performs the V operation.
 * 
 * Parameters:
 *   on - 1 for P operation, 0 for V operation
 *   semAddress - pointer to the semaphore
 */
void mutex(int on, int *semAddress) {
    if (on) {
        SYSCALL(PASSEREN, (unsigned int) semAddress, 0, 0);
    } else {
        SYSCALL(VERHOGEN, (unsigned int) semAddress, 0, 0);
    }
}

/******************************************************************************
 * Function: toggleInterrupts
 * 
 * This function enables or disables interrupts based on the value of the
 * 'enable' parameter. If 'enable' (1), it enables interrupts, otherwise it
 * disables them.
 * 
 * Parameters:
 *   enable - 1 to enable interrupts, 0 to disable them
 */
void toggleInterrupts(int enable) {
    unsigned int status = getSTATUS();
    
    if (enable) {
        setSTATUS(status | IECON);
    } else {
        setSTATUS(status & IECOFF);
    }
}

/******************************************************************************
 * Function: updateTLB
 * 
 * This function updates the TLB entry for a given page table entry. It first
 * sets the ENTRYHI register with the page number, then performs a TLB probe
 * (TLBP) to check if the entry is already present. If not, it sets the ENTRYLO
 * register with the frame address and writes the new entry into the TLB.
 * 
 * Parameters:
 *   ptEntry - pointer to the page table entry to be updated
 */
HIDDEN void updateTLB(ptEntry_t *ptEntry) {
    setENTRYHI(ptEntry->entryHI);
    TLBP(); /* probe TLB for entry as loaded above */

    if ((INDEX_PMASK & getINDEX()) == 0) {
        /* entry found (valid - P bit: 0), update TLB */ 
        setENTRYLO(ptEntry->entryLO); 
        TLBWI();
    }
}

/******************************************************************************
 * Function: pickVictim
 * 
 * This function selects a victim frame using round-robin replacement. It
 * increments the next victim index and wraps around if it exceeds the pool size.
 * 
 * Returns:
 *   The index of the selected victim frame.
 */
HIDDEN int pickVictim() {
    HIDDEN int nextVictim;
    nextVictim = (nextVictim + 1) % (POOLSIZE);
    return nextVictim;
}

/******************************************************************************
 * Function: supTlbExceptionHandler (the Pager)
 * 
 * This function handles TLB exceptions. It first checks if the exception is a
 * TLB modification exception, in which case it calls the program trap handler.
 * It then acquires a mutex over the swap pool and checks for a missing page.
 * If a victim frame is occupied, it writes the victim page to the backing store
 * and reads the requested page from the backing store. Finally, it updates the
 * swap pool entry and TLB with the new page table entry.
 */
void supTlbExceptionHandler() {
    /* 1. get support structure pointer */
    support_t *supportPtr = (support_t *) SYSCALL(GETSUPPORT, 0, 0, 0);
    state_PTR excState = &(supportPtr->sup_exceptState[PGFAULTEXCEPT]);

    /* 2. get cause of exception */
    int cause = CAUSE_GET_EXCCODE(excState->s_cause);
    
    /* 3. if tlb modification exception, program trap */
    if (cause == TLBMOD) {
        supProgramTrapHandler();
    }

    /* 4. get mutex over swap pool */
    mutex(ON, &swapPoolSem);

    /* 5. get missing page number */
    int missingPage = (excState->s_entryHI & VPNMASK) >> VPNSHIFT;
    missingPage %= MAXPAGES;
    
    /* 6. select victim frame using round-robin */
    int victimIndex = pickVictim();
    int frameAddr = POOLBASEADDR + (victimIndex * PAGESIZE);
    
    /* 7. check if victim frame occupied */
    if (swapPool[victimIndex].swap_asid != FREEFRAME) {
        /* 8. if occupied: */
        int victimAsid = swapPool[victimIndex].swap_asid;
        int victimPage = swapPool[victimIndex].swap_pageNo;
        int devNo = victimAsid - 1;

        toggleInterrupts(OFF);
        
        /* (a) mark page as invalid in owner's page table */
        swapPool[victimIndex].swap_ptePtr->entryLO &= VALIDOFF;
        
        /* (b) update TLB */
        updateTLB(swapPool[victimIndex].swap_ptePtr);

        toggleInterrupts(ON);
        
        /* (c) write victim page to backing store */
        int status = flashOperation(FLASH_WRITEBLK, devNo, victimPage, frameAddr);
        if (status != READY) {
            /* terminate if flash write fails */
            supProgramTrapHandler();
        }
    }
    
    /* 9. read requested page from backing store */
    int asid = supportPtr->sup_asid;
    int devNo = asid - 1;
    int status = flashOperation(FLASH_READBLK, devNo, missingPage, frameAddr);
    
    if (status != READY) {
        supProgramTrapHandler();
    }
    
    /* 10. update swap pool entry */
    swapPool[victimIndex].swap_asid = asid;
    swapPool[victimIndex].swap_pageNo = missingPage;
    swapPool[victimIndex].swap_ptePtr = &(supportPtr->sup_privatePgTbl[missingPage]);

    toggleInterrupts(OFF);

    /* 11. update page table entry for missing page to valid and dirty */
    supportPtr->sup_privatePgTbl[missingPage].entryLO = frameAddr | VALIDON | DIRTYON;

    /* 12. update TLB */
    updateTLB(&(supportPtr->sup_privatePgTbl[missingPage]));

    toggleInterrupts(ON);

    /* 13. release mutex over swap pool */
    mutex(OFF, &swapPoolSem);

    /* 14. return control to retry faulting instruction */
    LDST(excState);
}

/******************************************************************************
 * Function: markAllFramesFree
 * 
 * This function marks all frames in the swap pool as free for a given ASID.
 * It iterates through the swap pool and sets the ASID of each entry to
 * unoccupied (-1) if it matches the given ASID.
 * 
 * Parameters:
 *   asid - the ASID of the process whose frames are to be marked free
 */
void markAllFramesFree(int asid) {
    int i;
    for (i = 0; i < POOLSIZE; i++) {
        if (swapPool[i].swap_asid == asid) {
            swapPool[i].swap_asid = FREEFRAME;
        }
    }
}
