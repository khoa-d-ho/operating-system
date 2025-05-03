#include "../h/delayDaemon.h"

/* Local variables */
HIDDEN delayd_t delayd_table[UPROCMAX + 2]; 
HIDDEN delayd_t *delaydFree_h;
HIDDEN delayd_t *delayd_h; 
HIDDEN int adl_sem = 1; 

/* Helper functions */
HIDDEN void delayd_freeNode(delayd_t *node) { /*same*/
    node->d_next = delaydFree_h;
    delaydFree_h = node;
}

HIDDEN delayd_t *delayd_allocNode() { /*same*/
    if (delaydFree_h == NULL) {
        return NULL;
    }
    delayd_t* node = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    node->d_next = NULL;
    node->d_wakeTime = 0;
    node->d_supStruct = NULL;

    return node;
}

HIDDEN void delayd_insertADL(delayd_t *node) {
    delayd_t *prev = delayd_h;
    delayd_t *curr = delayd_h->d_next;
    while (curr != NULL && curr->d_wakeTime <= node->d_wakeTime) {
        prev = curr;
        curr = curr->d_next;
    }
    node->d_next = curr;
    prev->d_next = node;
}

HIDDEN void processADL() {
    cpu_t now;
    STCK(now);  /* Get current time-of-day in microseconds */

    delayd_t* prev = delayd_h;         /* Start from dummy head */
    delayd_t* curr = delayd_h->d_next; /* First real node (or dummy tail) */

    /* Traverse until we hit the dummy tail or a node that isn't ready to wake */
    while (curr != NULL && curr->d_wakeTime <= now) {
        /* Save next before we free current node */
        delayd_t* next = curr->d_next;

        /* Wake up the U-proc by performing SYS4 (V) on its private semaphore */
        mutex(0, &(curr->d_supStruct->sup_privateSem));

        /* Remove curr from ADL */
        prev->d_next = next;

        /* Return node to free list */
        delayd_freeNode(curr);

        /* Move to the next node */
        curr = next;
    }
}

/****************************************************************************
 * Function: initADL
 * 
 * This function initializes the Active Delay List (ADL) and launches the Delay
 * Daemon. It creates dummy head (0) and tail (MAXINT) nodes for the ADL and 
 * initializes the free list of delayd_t nodes.
 */
void initADL(void) {
    delayd_h = &(delayd_table[0]);
    delayd_h->d_wakeTime = 0;
    delayd_h->d_supStruct = NULL;

    delayd_h->d_next = &(delayd_table[UPROCMAX + 1]);
    delayd_h->d_next->d_wakeTime = MAXINT;
    delayd_h->d_next->d_supStruct = NULL;
    delayd_h->d_next->d_next = NULL;

    delaydFree_h = &delayd_table[1];
    delayd_t* temp = delaydFree_h;
    
    int i;
    for (i = 2; i <= UPROCMAX; i++) {
        temp->d_next = &delayd_table[i];
        temp = temp->d_next;
    }
    temp->d_next = NULL;
    
    memaddr ramtop;
    RAMTOP(ramtop);
    state_t daemonState;

    daemonState.s_pc = (memaddr) delayDaemon;
    daemonState.s_t9 = (memaddr) delayDaemon;
    daemonState.s_sp = ramtop - FRAMESIZE; 
    daemonState.s_status = ALLOFF | IEPON | IMON | TEBITON;
    daemonState.s_entryHI = (DELAY_ASID << ASIDSHIFT);

    int result = SYSCALL(CREATEPROCESS, (int)&daemonState, 0, 0); 

    if (result != OK) {
        PANIC(); 
    }
}

/****************************************************************************
 * Function: delayDaemon
 * 
 * This function implements the Delay Daemon. It waits for a signal from the
 * ADL and processes the ADL by waking up U-processes whose delay time has
 * expired. It also handles the mutual exclusion for the ADL using a semaphore.
 */
void delayDaemon() {
    while (TRUE) {
        SYSCALL(WAITFORCLOCK, 0, 0, 0); 

        mutex(1, &adl_sem);
        processADL();
        mutex(0, &adl_sem);
    }
}

/****************************************************************************
 * Function: delayFacility
 * 
 * This function implements the SYS18 (Delay) syscall. It creates a new node
 * in the ADL with the given delay time and adds it to the ADL. It also sets
 * up the support structure for the U-process.
 */
void delayFacility(support_t *supportPtr) {
    int delayDuration = supportPtr->sup_exceptState[GENERALEXCEPT].s_a1;

    if (delayDuration < 0) {
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }

    cpu_t now;
    STCK(now);  

    mutex(1, &adl_sem);  
    delayd_t *node = delayd_allocNode();
    if (node == NULL) {
        mutex(0, &adl_sem);
        SYSCALL(TERMPROCESS, 0, 0, 0); 
    }

    node->d_supStruct = supportPtr;
    node->d_wakeTime = now + ((cpu_t)delayDuration * MICROSECONDS);
    delayd_insertADL(node);

    toggleInterrupts(OFF);
    mutex(0, &adl_sem);
    mutex(1, &(supportPtr->sup_privateSem));
    toggleInterrupts(ON);
}

