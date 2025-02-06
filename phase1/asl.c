#include "../h/asl.h"
#include "../h/pcb.h"

/************************** ASL.C *******************************************
*
*  The implementation file for the Active Semaphore List. 
*  
*  The ASL is a list of semd_t nodes, each of which contains a
*  semaphore address (semAdd) and a pointer to a queue of processes. 
*  The list is ordered by semAdd in ascending order.
*  - insertBlocked: inserts a process in the queue of the semaphore
*  - removeBlocked: removes a process from the queue of the semaphore
*  - outBlocked: removes a process from the queue of the semaphore
*  - headBlocked: returns the first process in the queue of the semaphore
*  - initASL: initializes the ASL
*  Helper function: traverseASL, which traverses the ASL to find the
*  correct position to insert a semaphore.
*
*  Written by Khoa Ho & Hieu Tran
*  February 2025
****************************************************************************/


/* Global variables */
HIDDEN semd_t *semd_h;              /* Head of the active semaphore list */ 
HIDDEN semd_t *semdFree_h;          /* Head of the free semaphore list */
HIDDEN semd_t semdTable[MAXPROC+2]; /* Array of semaphores */

/****************************************************************************
* Helper function: traverseASL
* Traverse the ASL to find the correct position to insert a semaphore.
* Returns the semaphore node with the given semAdd.
* If the semaphore node does not exist, returns NULL.
*/
HIDDEN semd_t *traverseASL(int *semAdd, semd_t **prev) {
    *prev = semd_h;  
    semd_t *curr = semd_h->s_next; /* Skip the first dummy node */
    while (curr != NULL && curr->s_semAdd < semAdd) {
        *prev = curr;
        curr = curr->s_next;
    }
    return curr;
}

/****************************************************************************
 * Function: insertBlocked
 * Inserts a process in the queue of the semaphore with the given semAdd.
 * If the semaphore does not exist, creates a new semaphore node.
 * Returns TRUE if the semaphore is not found.
 */
int insertBlocked(int *semAdd, pcb_PTR p) {
    semd_t *prev; /* Pointer to the previous semaphore node */
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd) {
        /* Semaphore does not exist */
        if (semdFree_h == NULL)
            /* No more free semaphores */
            return TRUE;
        
        /* Get a free semaphore */
        sem = semdFree_h;
        semdFree_h = sem->s_next;
        sem->s_semAdd = semAdd;
        sem->s_procQ = mkEmptyProcQ();
        /* Insert new semaphore in correct position */ 
        sem->s_next = prev->s_next;
        prev->s_next = sem;
    }
    insertProcQ(&(sem->s_procQ), p);
    return FALSE;
}

/****************************************************************************
 * Function: removeBlocked
 * Removes the head pcb from the queue of the semaphore with the given semAdd.
 * If the semaphore does not exist, returns NULL.
 */
pcb_PTR removeBlocked(int *semAdd) {
    semd_t *prev;
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd) {
        /* Semaphore does not exist */
        return NULL;
    }
    pcb_PTR p = removeProcQ(&(sem->s_procQ));
    p->p_semAdd = NULL;

    if (emptyProcQ(sem->s_procQ)) {
        /* If the semaphore queue becomes empty, remove the semaphore */
        prev->s_next = sem->s_next;
        sem->s_next = semdFree_h;
        semdFree_h = sem;
    }
    return p;
}

/****************************************************************************
 * Function: outBlocked
 * Removes the pcb pointed to by p from the process queue associated with 
 * p's semaphore.
 * If the semaphore does not exist, returns NULL.
 */
pcb_PTR outBlocked(pcb_PTR p) {
    semd_t *prev = semd_h;
    semd_t *sem = semd_h->s_next;
    while (sem != NULL) {
        /* Traverse the ASL */
        pcb_PTR removed = outProcQ(&(sem->s_procQ), p);
        if (removed != NULL) {
            /* If the process is found */
            if (emptyProcQ(sem->s_procQ)) {
                /* If the semaphore queue becomes empty, remove the semaphore */
                prev->s_next = sem->s_next;
                sem->s_next = semdFree_h;
                semdFree_h = sem;
            }
            return removed;
        }
        /* Move to the next semaphore */
        prev = sem;
        sem = sem->s_next;
    }
    return NULL;
}

/****************************************************************************
 * Function: headBlocked
 * Returns the head pcb from the queue of the semaphore with the given semAdd.
 * If the semaphore does not exist or the queue is empty, returns NULL.
 */
pcb_PTR headBlocked(int *semAdd) {
    semd_t *prev;
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd || emptyProcQ(sem->s_procQ)) {
        /* Semaphore does not exist or the queue is empty */
        return NULL;
    }
    return headProcQ(sem->s_procQ);
}

/****************************************************************************
 * Function: initASL
 * Initializes the ASL.
 * The ASL contains a dummy node at the beginning and the end of the list.
 * The ASL also contains a list of free semd_t nodes.
 */
void initASL() {
    /* Initialize the active semaphore list */
    semd_h = &(semdTable[0]);
    semd_h->s_semAdd = (int*)(0x00000000); /* 0 for first dummy node */
    semd_h->s_procQ = NULL;

    semd_h->s_next = &(semdTable[MAXPROC+1]);
    semd_h->s_next->s_procQ = NULL;
    semd_h->s_next->s_next = NULL;
    semd_h->s_next->s_semAdd = (int*)MAXINT; /* MAXINT for last dummy node */

    /* Initialize the free semaphore list */
    semdFree_h = &semdTable[1];
    semd_t *temp = semdFree_h;
    int i;
    for (i = 2; i < MAXPROC+1; i++) {
        temp->s_next = &(semdTable[i]);
        temp->s_procQ = NULL;
        temp = temp->s_next;
    }
    temp->s_procQ = NULL;
    temp->s_next = NULL;
}