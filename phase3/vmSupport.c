#include "../h/vmSupport.h"

swap_t swapPool[POOLSIZE];
int swapPoolSem;         

void toggleInterrupts(int enable) {
    unsigned int status = getSTATUS();
    
    if (enable) {
        setSTATUS(status | IECON);
    } else {
        setSTATUS(status & IECOFF);
    }
}

void update_tlb_handler(ptEntry_t *ptEntry) {
    setENTRYHI(ptEntry->entryHI); /* Load the page of interest's virtual page number (VPN) and ASID into EntryHi*/
    TLBP(); /*probe the TLB to searches for a matching entry using the current EntryHi*/

    /*In Index CP0 control register, check INDEX.P bit (bit 31 of INDEX). We perform bitwise AND with 0x8000000 to isolate bit 31*/
    if ((KUSEG & getINDEX()) == 0){ /*If P bit == 0 -> found a matching entry. Else P bit == 1 if not found*/
        setENTRYLO(ptEntry->entryLO); /*set content of entryLO to write to Index.TLB-INDEX*/
        TLBWI(); /* Write content of entryHI and entryLO CP0 registers into Index.TLB-INDEX -> This updates the cached entry to match the page table*/
    }
}

/* rr */
int pickVictim() {
    int nextVictim = 0;
    nextVictim = (nextVictim + 1) % POOLSIZE;
    return nextVictim;
}

int flashIO(int operation, int devNo, int blockNo, int frameAddr) {
    int devIndex = ((FLASHINT - DISKINT) * DEVPERINT) + devNo;
    
    /* get device registers */
    devregarea_t *devRegArea = (devregarea_t *) RAMBASEADDR;

    mutex(TRUE, (int *) &devSemaphore[devIndex]); /* get mutex for device */

    devRegArea->devreg[devIndex].d_data0 = frameAddr;
    toggleInterrupts(FALSE);
    devRegArea->devreg[devIndex].d_command = (blockNo << BITSHIFT_8) | operation;
    int status = SYSCALL(WAITFORIO, FLASHINT, devNo, (operation == READBLK));
    toggleInterrupts(TRUE);

    mutex(FALSE, (int *) &devSemaphore[devIndex]);

    if (status != READY) {
        /* handle error */
        status = -status;
    }

    return status;
}

void mutex(int yes, int *semAddress) {
    if (yes) {
        SYSCALL(PASSEREN, (unsigned int) semAddress, 0, 0);
    } else {
        SYSCALL(VERHOGEN, (unsigned int) semAddress, 0, 0);
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
void debug9 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug10 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void debug11 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void supTlbExceptionHandler() {
    debug9(0, 0, 0, 0);
    support_t *supportPtr = (support_t *) SYSCALL(GETSUPPORT, 0, 0, 0);
    debug10(0, 0, 0, 0);

    int cause = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_cause & CAUSE_EXCCODE_MASK) >> CAUSE_EXCCODE_SHIFT;
    
    /* if tlb modification exception, program trap */
    if (cause == TLBMOD) {
        supProgramTrapHandler();
    }

    /* get mutex over swap pool */
    mutex(1, &swapPoolSem);

    /* get missing page number */
    int missingPage = (supportPtr->sup_exceptState[PGFAULTEXCEPT].s_entryHI & VPNMASK) >> VPNSHIFT;
    missingPage %= MAXPAGES;
    
    /* select victim frame using round-robin */
    int victimIndex = pickVictim();
    int frameAddr = POOLBASEADDR + (victimIndex * PAGESIZE);
    
    /* check if victim frame occupied */
    if (swapPool[victimIndex].swap_asid != FREEFRAME) {
        /* disable interrupts while updating page tables */
        toggleInterrupts(FALSE);
        
        /* mark page as invalid in owner's page table */
        swapPool[victimIndex].swap_ptePtr->entryLO = swapPool[victimIndex].swap_ptePtr->entryLO & VALIDOFF;
        
        /* clear tlb to force reload of updated entries */
        update_tlb_handler(swapPool[victimIndex].swap_ptePtr);
        
        /* reenble interrupts */
        toggleInterrupts(TRUE);
        
        /* save victim page to backing store */
        int victimAsid = swapPool[victimIndex].swap_asid;
        int victimPage = swapPool[victimIndex].swap_pageNo;
        int devNo = victimAsid - 1;

        int status = flashIO(WRITEBLK, devNo, victimPage, frameAddr);
        if (status != READY) {
            /* terminate if flash operation fails */
            supProgramTrapHandler();
        }
    }
    
    /* read requested page from backing store */
    int asid = supportPtr->sup_asid;
    int devNo = asid - 1;
    int status = flashIO(READBLK, devNo, missingPage, frameAddr);
    
    if (status != READY) {
        debug11(0, 0, 0, 0);
        /* terminate if flash operation fails */
        supProgramTrapHandler();
    }
    
    /* update swap pool entry */
    swapPool[victimIndex].swap_asid = asid;
    swapPool[victimIndex].swap_pageNo = missingPage;
    swapPool[victimIndex].swap_ptePtr = &(supportPtr->sup_privatePgTbl[missingPage]);

    toggleInterrupts(FALSE);
    supportPtr->sup_privatePgTbl[missingPage].entryLO = frameAddr | VALIDON | DIRTYON;
    update_tlb_handler(&(supportPtr->sup_privatePgTbl[missingPage]));
    toggleInterrupts(TRUE);

    mutex(0, &swapPoolSem);

    /* return to the exception state to retry instruction */
    LDST(&(supportPtr->sup_exceptState[PGFAULTEXCEPT]));
}


