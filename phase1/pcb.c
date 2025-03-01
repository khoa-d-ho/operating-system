#include "../h/pcb.h" 

/************************** PCB.C ******************************************
*
*  The implementation file for the Process Control Block Module.
*
*  The PCB module manages process control blocks and their relationships.
*  Each PCB contains process state information and maintains process
*  relationships through various pointer fields.
*
*  Key functions:
*  - allocPcb: allocates and returns a free PCB
*  - freePcb: returns a PCB to the free list
*  - initPcbs: initializes the PCB free list
*  - insertProcQ/removeProcQ: manage process queues
*  - insertChild/removeChild: manage process hierarchies
*  - outChild/outProcQ: remove specific processes
*  - mkEmptyProcQ/emptyProcQ: create/check empty queues
*  - headProcQ: peek at queue head
*  Helper function: resetPcb, which initializes PCB fields to default values
*
*  Written by Khoa Ho & Hieu Tran
*  February 2025
****************************************************************************/

/* Global variables */
HIDDEN pcb_t pcbFree_list[MAXPROC]; /* Static array to store MAXPROC pcbs */
HIDDEN pcb_PTR pcbFree_h;     /* Pointer to the head of the free pcb list */

/****************************************************************************
* Helper function: resetPcb
* Resets a process control block (pcb) by clearing its fields.
* Ensures no residual values persist when reusing pcbs.
*/
HIDDEN void resetPcb(pcb_PTR p) {
    if (p == NULL)
        return;

    /* Reset all PCB pointer fields to NULL */
    p->p_next = NULL;
    p->p_prev = NULL;
    p->p_prnt = NULL;
    p->p_child = NULL;
    p->p_next_sib = NULL;
    p->p_prev_sib = NULL;
    
    /* Reset all processor state fields */
    p->p_s.s_entryHI = 0;
    p->p_s.s_cause = 0;
    p->p_s.s_status = 0;
    p->p_s.s_pc = 0;

    p->p_time = 0;   /* Reset process execution time */
    p->p_semAdd = NULL; /* Clear semaphore address reference */

    /* Reset all general-purpose register values */
    int i;
    for (i = 0; i < STATEREGNUM; i++) {
        p->p_s.s_reg[i] = 0;
    }
}

/****************************************************************************
 * Function: freePcb
 * Insert the element pointed to by p onto the pcbFree list. 
 * The pcb is inserted at the head of the list.
 */

void freePcb(pcb_PTR p) {
    if (p == NULL) 
        return;
    
    resetPcb(p); /* Clear the pcb data before reusing */

    /* Add the pcb back to the free list */
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

/****************************************************************************
 * Function: allocPcb
 * Return NULL if the pcbFree list is empty. Otherwise, remove an element 
 * from the pcbFree list, provide initial values for ALL of the pcbs fields 
 * (i.e. NULL and/or 0) and then return a pointer to the removed element. 
 * Note: pcbs get reused, so it is important that no previous value persist 
 * in a pcb when it gets reallocated. 
 */
pcb_PTR allocPcb() {
    if (pcbFree_h == NULL)
        return NULL;  /* No available pcb in free list */

    pcb_PTR p = pcbFree_h; /*  Get the first available pcb */
    pcbFree_h = pcbFree_h->p_next; /*  Move head to next free pcb */
    resetPcb(p);  /* Initialize the allocated pcb */
    
    return p;
}

/****************************************************************************
 * Function: initPcbs
 * Initialize the pcbFree list to contain all the elements of the static 
 * array of MAXPROC pcbs. This method should be called only once during 
 * data structure initialization. 
 */
void initPcbs() {
    pcb_PTR tmp;
    pcbFree_h = &pcbFree_list[0]; /* Point to the first pcb in the array */
    pcbFree_h->p_prev = NULL;
    tmp = pcbFree_h;

    /* Link all pcbs in a free list */
    int i;
    for (i = 1;  i < MAXPROC; i++) {
        tmp->p_next = &pcbFree_list[i];
        tmp = tmp->p_next;
        tmp->p_prev = NULL;
    }
    tmp->p_next = NULL; /* Mark end of the free list */
}

/****************************************************************************
 * Function: mkEmptyProcQ
 * This method is used to initialize a variable to be tail pointer to a 
 * process queue.
 * Return a pointer to the tail of an empty process queue; i.e. NULL.
 */
pcb_PTR mkEmptyProcQ() {
    return NULL;
}

/****************************************************************************
 * Function: emptyProcQ
 * Return TRUE if the queue whose tail is pointed to by tp is empty.
 * Return FALSE otherwise.
 */
int emptyProcQ (pcb_PTR tp) {
    return (tp == NULL);
}

/****************************************************************************
 * Function: insertProcQ
 * Insert the pcb pointed to by p into the process queue whose tail- pointer 
 * is pointed to by tp. 
 * Note: The double indirection through tp to allow for the possible updating 
 * of the tail pointer as well.
 */
void insertProcQ (pcb_PTR *tp, pcb_PTR p) {
    if (emptyProcQ(*tp)) {
        /* If queue is empty, insert the first element */
        *tp = p;
        p->p_next = *tp;
        p->p_prev = *tp;
    } else {
        /* Insert the new element after the current tail */
        p->p_next = (*tp)->p_next;
        p->p_prev = *tp;
        (*tp)->p_next->p_prev = p;
        (*tp)->p_next = p;
        *tp = p; /* Update tail pointer */
    }
}

/****************************************************************************
 * Function: removeProcQ
 * Remove the first (i.e. head) element from the process queue whose 
 * tail-pointer is pointed to by tp. 
 * Return NULL if the process queue was initially empty; otherwise return the 
 * pointer to the removed element. Update the process queue’s tail pointer 
 * if necessary. 
 */
pcb_PTR removeProcQ (pcb_PTR *tp) {
    if (emptyProcQ(*tp)) 
        return NULL;    /* Empty queue */

    return outProcQ(tp, (*tp)->p_next); /* Remove the head */
}

/****************************************************************************
 * Function: outProcQ
 * Remove the pcb pointed to by p from the process queue whose tail-pointer 
 * is pointed to by tp. Update the process queue’s tail pointer if necessary. 
 * If the desired entry is not in the indicated queue (an error condition), 
 * return NULL; otherwise, return p. 
 * Note: p can point to any element of the process queue. 
 */
pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p) {
    if (emptyProcQ(*tp) || p == NULL)
        return NULL;    /* Invalid input */

    /* Ensure p is in the list */
    pcb_PTR temp = *tp;
    int found = 0;
    do {
        if (temp == p) {
            found = 1;
            break;
        }
        temp = temp->p_next;
    } while (temp != *tp);

    if (!found)
        return NULL;

    if (p->p_prev == p && p->p_next == p)
        /* If removing the only element in the queue */
        *tp = NULL;
    else {
        /* Adjust pointers to remove p from the queue */
        p->p_next->p_prev = p->p_prev;
        p->p_prev->p_next = p->p_next;
        if (*tp == p)
            *tp = p->p_prev;  /* Update tail if it was the removed element */
    }

    /* Clear links of the removed element */
    p->p_next = p->p_prev = NULL;
    return p;
}

/****************************************************************************
 * Function: headProcQ
 * Return the pointer to the first pcb from the process queue whose tail is 
 * pointed to by tp. This pcb will not be removed from the process queue.
 * Return NULL if the process queue is empty.
 */
pcb_PTR headProcQ(pcb_PTR tp) {
    if (emptyProcQ(tp))
        return NULL;
    return tp->p_next;
}

/****************************************************************************
 * Function: emptyChild
 * Return TRUE if the pcb pointed to by p has no children.
 * Return FALSE otherwise.
 */
int emptyChild (pcb_PTR p) {
    return p->p_child == NULL;
}

/****************************************************************************
 * Function: insertChild
 * Make the pcb pointed to by p a child of the pcb pointed to by prnt.
 */
void insertChild (pcb_PTR prnt, pcb_PTR p) {
    insertProcQ(&(prnt->p_child), p);
    p->p_prnt = prnt;   /* Set parent pointer */
}

/****************************************************************************
 * Function: removeChild
 * Make the first child of the pcb pointed to by p no longer a child of p. 
 * Return NULL if initially there were no children of p. 
 * Otherwise, return a pointer to this removed first child pcb.
 */
pcb_PTR removeChild (pcb_PTR p) {
    return removeProcQ(&(p->p_child));
}

/****************************************************************************
 * Function: outChild
 * Make the pcb pointed to by p no longer the child of its parent. 
 * If the pcb pointed to by p has no parent, return NULL; 
 * otherwise, return p. 
 * Note: The element pointed to by p need not be the first child of 
 * its parent.
 */
pcb_PTR outChild (pcb_PTR p) {
    pcb_PTR prnt = p->p_prnt;
    if (prnt == NULL) 
        return NULL; /* No parent, cannot remove */
    
    return outProcQ(&(prnt->p_child), p);
}
