#include "../h/delayDaemon.h"

/****************************************************************************
 * delayDaemon.c
 *
 * This module implements the Delay Daemon and the Active Delay List (ADL).
 * The ADL is a linked list of delay nodes that keeps track of U-processes
 * that are blocked for a specified delay duration. The Delay Daemon processes 
 * the ADL and wakes up U-processes when their delay time has expired.
 *
 * Written by: Hieu Tran and Khoa Ho
 * May 2025
 ****************************************************************************/

/* Local variables */
HIDDEN delayd_t delayd_table[UPROCMAX + 2]; /* ADL table with dummy head and tail */
HIDDEN delayd_t *delaydFree_h;              /* free list of delayd_t nodes */
HIDDEN delayd_t *delayd_h;                  /* head of the ADL */
HIDDEN int adl_sem = 1;                     /* semaphore for ADL mutual exclusion */

/* Helper functions */
/****************************************************************************
 * Function: delayd_freeNode
 * 
 * This function adds a delayd_t node to the free list. It updates the next
 * pointer of the node to point to the head of the free list and sets the head
 * of the free list to the node.
 * 
 * Parameters:
 *   node - The delayd_t node to be added to the free list.
 */
HIDDEN void delayd_freeNode(delayd_t *node) { 
    node->d_next = delaydFree_h;
    delaydFree_h = node;
}

/****************************************************************************
 * Function: delayd_allocNode
 * 
 * This function allocates a delayd_t node from the free list. It updates the
 * head of the free list to point to the next node in the list and returns the
 * allocated node.
 * 
 * Returns:
 *   A pointer to the allocated delayd_t node, or NULL if no nodes are available.
 */
HIDDEN delayd_t *delayd_allocNode() {
    if (delaydFree_h == NULL) {
        /* no free nodes available */
        return NULL;
    }
    /* allocate a node from the free list */
    delayd_t* node = delaydFree_h;
    delaydFree_h = delaydFree_h->d_next;

    /* initialize the node */
    node->d_next = NULL;
    node->d_wakeTime = 0;
    node->d_supStruct = NULL;

    return node;
}

/****************************************************************************
 * Function: delayd_insertADL
 * 
 * This function inserts a delayd_t node into the ADL in sorted order based on 
 * the wake time. It traverses the list to find the correct position for the 
 * new node and updates the next pointers accordingly.
 * 
 * Parameters:
 *   node - The delayd_t node to be inserted into the ADL.
 */
HIDDEN void delayd_insertADL(delayd_t *node) {
    delayd_t *prev = delayd_h; /* dummy head */
    delayd_t *curr = delayd_h->d_next; /* first real node */

    /* traverse the list to find the correct position for the new node */
    while (curr != NULL && curr->d_wakeTime <= node->d_wakeTime) {
        prev = curr;
        curr = curr->d_next;
    }
    node->d_next = curr;
    prev->d_next = node;
}

/****************************************************************************
 * Function: processADL
 * 
 * This function processes the ADL by waking up U-processes whose delay time 
 * has expired. It traverses the ADL and wakes up each U-process by performing 
 * SYS4 (V) on its private semaphore. It also removes the node from the ADL and 
 * returns it to the free list.
 */
HIDDEN void processADL() {
    /* get current time of day in microseconds */
    cpu_t now;
    STCK(now);

    delayd_t* prev = delayd_h;         
    delayd_t* curr = delayd_h->d_next; 

    /* traverse the ADL and process all nodes whose wake time has expired */    
    while (curr != NULL && curr->d_wakeTime <= now) {
        /* save next before we free current node */
        delayd_t* next = curr->d_next;

        /* wake up the U-proc by performing V on its private semaphore */
        mutex(OFF, &(curr->d_supStruct->sup_privateSem));

        /* remove curr from ADL */
        prev->d_next = next;

        /* return node to free list */
        delayd_freeNode(curr);

        /* move to the next node */
        curr = next;
    }
}

/* Global functions */
/****************************************************************************
 * Function: initADL
 * 
 * This function initializes the ADL and launches the Delay Daemon. 
 * It creates dummy head (0) and tail (MAXINT) nodes for the ADL and 
 * initializes the free list of delayd_t nodes.
 */
void initADL() {
    /* initialize the ADL with dummy head and tail */
    delayd_h = &(delayd_table[0]);
    delayd_h->d_wakeTime = 0;
    delayd_h->d_supStruct = NULL;

    delayd_h->d_next = &(delayd_table[UPROCMAX + 1]);
    delayd_h->d_next->d_wakeTime = MAXINT;
    delayd_h->d_next->d_supStruct = NULL;
    delayd_h->d_next->d_next = NULL;

    /* initialize the free list of delayd_t nodes */
    delaydFree_h = &delayd_table[1];
    delayd_t* temp = delaydFree_h;
    
    int i;
    for (i = 2; i <= UPROCMAX; i++) {
        temp->d_next = &delayd_table[i];
        temp = temp->d_next;
    }
    temp->d_next = NULL;
    
    /* create the Delay Daemon process */
    memaddr ramtop;
    RAMTOP(ramtop);
    state_t daemonState;

    daemonState.s_pc = (memaddr) delayDaemon;
    daemonState.s_t9 = (memaddr) delayDaemon;
    daemonState.s_sp = ramtop - FRAMESIZE; 
    daemonState.s_status = ALLOFF | IEPON | IMON | TEBITON;
    daemonState.s_entryHI = (DELAY_ASID << ASIDSHIFT);

    int result = SYSCALL(CREATEPROCESS, (int)&daemonState, 0, 0); 

    /* terminate if creation failed */
    if (result != OK) {
        supProgramTrapHandler();  
    }
}

/****************************************************************************
 * Function: delayDaemon
 * 
 * This function implements the Delay Daemon, a kernel-level process that
 * periodically checks the ADL to wake up delayed U-processes whose wait time 
 * has expired.
 * 
 * It waits for a pseudo-clock tick (every 100 ms), then gains mutual
 * exclusion over the ADL using a semaphore. It processes expired delay
 * events by performing a V on the associated U-proc's private
 * semaphore and returns the event descriptors to the free list.
 */
void delayDaemon() {
    while (TRUE) {
        /* wait for pseudoclock signal */
        SYSCALL(WAITFORCLOCK, 0, 0, 0); 

        mutex(ON, &adl_sem);
        processADL();
        mutex(OFF, &adl_sem);
    }
}

/****************************************************************************
 * Function: delayFacility
 * 
 * This function implements the SYS18 (Delay) syscall. It creates a new node
 * in the ADL with the given delay time and adds it to the ADL. It also sets
 * up the support structure for the U-process.
 * 
 * The function performs the following steps:
 * 1. If the delay duration is negative, the U-proc is terminated.
 * 2. Allocates a delay event descriptor from the free list.
 *    If allocation fails, the U-proc is terminated.
 * 3. Calculates the wake-up time and inserts the descriptor into the
 *    ADL in order.
 * 4. Atomically releases mutual exclusion over the ADL and performs a 
 *    P on the U-proc’s private semaphore, blocking the U-proc.
 *    This ensures the process sleeps until the Delay Daemon wakes it.
 */
void delayFacility(support_t *supportPtr) {
    int delayDuration = supportPtr->sup_exceptState[GENERALEXCEPT].s_a1;

    /* check if delay duration is valid */
    if (delayDuration < 0) {
        supProgramTrapHandler();
    }

    cpu_t now;
    STCK(now);  

    /* allocate a delay event descriptor */
    mutex(ON, &adl_sem);  
    delayd_t *node = delayd_allocNode();
    if (node == NULL) {
        mutex(OFF, &adl_sem);
        supProgramTrapHandler();
    }

    /* set up the delay event descriptor */
    node->d_supStruct = supportPtr;
    node->d_wakeTime = now + ((cpu_t)delayDuration * MICROSECONDS);
    delayd_insertADL(node);

    toggleInterrupts(OFF);
    /* release mutual exclusion over the ADL & allow Delay Daemon to process it */
    mutex(OFF, &adl_sem);
    /* perform P on the U-proc's private semaphore to block it from running */
    mutex(ON, &(supportPtr->sup_privateSem));
    toggleInterrupts(ON);
}

