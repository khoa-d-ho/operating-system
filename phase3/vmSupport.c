#include "../h/vmSupport.h"


void initSwapStructs() {
    int i;
    
    /* initialize semaphore to 1 (mutex) */
    swapPoolSem = 1;
    
    /* initialize swap pool table entries */
    for (i = 0; i < POOLSIZE; i++) {
        swapPool[i].asid = FREEFRAME;
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
void tlbExceptionHandler() {
    support_t *supportPtr = (support_t *) SYSCALL(GETSUPPORT, 0, 0, 0);
    
    int cause = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_cause & CAUSE_EXCCODE_MASK) >> CAUSE_EXCCODE_SHIFT;
    
    /* if tlb modification exception, program trap */
    if (cause == TLBMOD) {
        programTrapHandler();
        return;
    }
    
    /* get mutex over swap pool */
    SYSCALL(PASSEREN, (int) &swapPoolSem, 0, 0);
    
    /* get missing page number */
    int missingPage = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI & VPNMASK) >> VPNSHIFT;
    missingPage %= MAXPAGES;
    
    /* select victim frame using round-robin */
    int victimIndex = pickVictim();
    int frameAddr = POOLBASEADDR + (victimIndex * PAGESIZE);
    
    /* check if victim frame occupied */
    if (swapPool[victimIndex].asid != FREEFRAME) {
        /* disable interrupts while updating page tables */
        unsigned int status = getSTATUS();
        setSTATUS(status & ~IECON);
        
        /* mark page as invalid in owner's page table */
        swapPool[victimIndex].ptePtr->entryLO &= VALIDOFF;
        
        /* clear tlb to force reload of updated entries */
        TLBCLR();
        
        /* reenble interrupts */
        setSTATUS(status);
        
        /* save victim page to backing store */
        int victimAsid = swapPool[victimIndex].asid;
        int victimPage = swapPool[victimIndex].pageNo;
        
        /* get flash dev no*/
        int devNo = victimAsid - 1;
        
        /* write victim page to flash */
        int status = flashIO(FLASHW, devNo, victimPage, frameAddr);
        if (status != READY) {
            /* free mutex and terminate if flash operation fails */
            SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
            programTrapHandler();
            return;
        }
    }
    
    /*read requested page from backing store */
    int asid = supportPtr->sup_asid;
    int devNo = asid - 1;
    int status = flashIO(FLASHR, devNo, missingPage, frameAddr);
    
    if (status != READY) {
        /* free mutex and terminate if flash operation fails */
        SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
        programTrapHandler();
        return;
    }
    
    /* update page table and swap pool table*/
    unsigned int savedStatus = getSTATUS();
    setSTATUS(savedStatus & ~IECON);
    
    /* update swap pool entry */
    swapPool[victimIndex].asid = asid;
    swapPool[victimIndex].pageNo = missingPage;
    swapPool[victimIndex].ptePtr = &(supportPtr->sup_privatePgTbl[missingPage]);
    
    /*update page table entry, mark as valid and dirty */
    supportPtr->sup_privatePgTbl[missingPage].entryLO = frameAddr | VALIDON | DIRTYON;
    
    TLBCLR();

    /* restore interrupt status */
    setSTATUS(savedStatus);
    
    /* release mutex */
    SYSCALL(VERHOGEN, (int) &swapPoolSem, 0, 0);
    
    /* return to the exception state to retry instruction */
    LDST(&(supportPtr->sup_exceptState[PGFAULTEXCEPT]));
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
    
    /* set data0 register to frame address */
    devRegArea->devreg[devIndex].d_data0 = frameAddr;
    
    /* disable interrupts for atomic operations */
    unsigned int savedStatus = getSTATUS();
    setSTATUS(savedStatus & ~IECON);
    
    /* set command register with block number and operation */
    devRegArea->devreg[devIndex].d_command = (blockNo << 8) | operation;
    
    /* i/o wait  */
    int status = SYSCALL(WAITFORIO, FLASHINT, devNo, 0);
    
    /* restore interrupts */
    setSTATUS(savedStatus);
    
    return status;
}
