#include "../h/vmSupport.h"

swap_t swapPool[POOLSIZE];
int swapPoolSem;      
int nextVictim = 0;   

void toggleInterrupts(int enable) {
    unsigned int status = getSTATUS();
    
    if (enable) {
        setSTATUS(status | IECON);
    } else {
        setSTATUS(status & ~IECON);
    }
}

/* rr */
int pickVictim() {
    nextVictim = (nextVictim + 1) % POOLSIZE;
    return nextVictim;
}

int flashIO(int operation, int devNo, int blockNo, int frameAddr) {
    int devIndex = ((FLASHINT - DISKINT) * DEVPERINT) + devNo;
    
    /* get device registers */
    devregarea_t *devRegArea = (devregarea_t *) RAMBASEADDR;

    devRegArea->devreg[devIndex].d_data0 = frameAddr;
    toggleInterrupts(FALSE);
    devRegArea->devreg[devIndex].d_command = (blockNo << BITSHIFT_8) | operation;
    int status = SYSCALL(WAITFORIO, FLASHINT, devNo, 0);
    toggleInterrupts(TRUE);

    if (status != READY) {
        /* handle error */
        status = -status;
    }

    return status;
}

/* HIDDEN void updateTLB(int victimIndex) {

} */

void mutex(int state, int *semAddress) {
    if (state == 1) {
        SYSCALL(PASSEREN, (int) semAddress, 0, 0);
    } else {
        SYSCALL(VERHOGEN, (int) semAddress, 0, 0);
    }
}

void initSwapStructs() {
    int i;
    
    /* initialize semaphore to 1 (mutex) */
    swapPoolSem = 1;
    
    /* initialize swap pool table entries */
    for (i = 0; i < POOLSIZE; i++) {
        swapPool[i].swap_asid = FREEFRAME;
    }
}

/*****************************************************************************
 * tlbExceptionHandler
 * 
 * 1. get support structure pointer via sys8 (getsupport)
 * 2. determine exception cause from sup_exceptstate[pgfaultexcept]
 * 3. if tlb modification exception, call programtraphandler
 * 4. gain mutual exclusion over swap pool (p)
 * 5. get missing page number from entryhi
 * 6. select victim frame using round-robin replacement
 * 7. check if victim frame is occupied
 * 8. if occupied:
 *    a. mark owner's page table entry invalid
 *    b. clear tlb to force reload of updated entries
 *    c. write victim page to backing store
 * 9. read requested page from backing store
 * 10. update swap pool entry with new page info
 * 11. update page table entry as valid and dirty
 * 12. clear tlb to ensure consistency
 * 13. release mutex (v)
 * 14. return control to retry faulting instruction
 *****************************************************************************/
void supTlbExceptionHandler() {
    support_t *supportPtr = (support_t *) SYSCALL(GETSUPPORT, 0, 0, 0);
    
    int cause = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_cause & CAUSE_EXCCODE_MASK) >> CAUSE_EXCCODE_SHIFT;
    
    /* if tlb modification exception, program trap */
    if (cause == TLBMOD) {
        supProgramTrapHandler();
    }

    /* get mutex over swap pool */
    mutex(1, (int *) &swapPoolSem);

    /* get missing page number */
    int missingPage = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI & VPNMASK) >> VPNSHIFT;
    missingPage %= MAXPAGES;
    
    /* select victim frame using round-robin */
    int victimIndex = pickVictim();
    
    /* check if victim frame occupied */
    if (swapPool[victimIndex].swap_asid != FREEFRAME) {
        /* disable interrupts while updating page tables */
        toggleInterrupts(FALSE);
        
        /* mark page as invalid in owner's page table */
        swapPool[victimIndex].swap_ptePtr->entryLO = swapPool[victimIndex].swap_ptePtr->entryLO & VALIDOFF;
        
        /* clear tlb to force reload of updated entries */
        TLBCLR();
        
        /* reenble interrupts */
        toggleInterrupts(TRUE);
        
        /* save victim page to backing store */
        int victimAsid = swapPool[victimIndex].swap_asid;
        int victimPage = swapPool[victimIndex].swap_pageNo;
        int devNo = victimAsid - 1;
        int frameAddr = POOLBASEADDR + (victimIndex * PAGESIZE);

        int status = flashIO(WRITEBLK, devNo, victimPage, frameAddr);
        if (status != READY) {
            /* free mutex and terminate if flash operation fails */
            mutex(0, (int *) &swapPoolSem);
            supProgramTrapHandler();
        }
    }
    
    /* read requested page from backing store */
    int asid = supportPtr->sup_asid;
    int devNo = asid - 1;
    int frameAddr = POOLBASEADDR + (missingPage * PAGESIZE);
    int status = flashIO(READBLK, devNo, missingPage, frameAddr);
    
    if (status != READY) {
        /* free mutex and terminate if flash operation fails */
        mutex(0, (int *) &swapPoolSem);
        supProgramTrapHandler();
    }
    
    /* update swap pool entry */
    swapPool[victimIndex].swap_asid = asid;
    swapPool[victimIndex].swap_pageNo = missingPage;
    swapPool[victimIndex].swap_ptePtr = &(supportPtr->sup_privatePgTbl[missingPage]);

    toggleInterrupts(FALSE);
    
    /*update page table entry, mark as valid and dirty */
    supportPtr->sup_privatePgTbl[missingPage].entryLO = frameAddr | VALIDON | DIRTYON;
    
    TLBCLR();

    /* restore interrupt status */
    toggleInterrupts(TRUE);

    /* release mutex */
    mutex(0, (int *) &swapPoolSem);

    /* return to the exception state to retry instruction */
    LDST(&(supportPtr->sup_exceptState[PGFAULTEXCEPT]));
}


