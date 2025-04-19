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
*
*  Written by Khoa Ho & Hieu Tran
*  February 2025
****************************************************************************/

HIDDEN pcb_PTR pcbFree_h; /* Pointer to the head of the free pcb list */
HIDDEN pcb_t pcbFree_list[MAXPROC];

/****************************************************************************
 * Function: freePcb
 * Insert the element pointed to by p onto the pcbFree list. 
 * The pcb is inserted at the head of the list.
 */

void freePcb(pcb_PTR p) {
	insertProcQ(&pcbFree_h, p);
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
	if(pcbFree_h == NULL) {
		return NULL; /* No available pcb in free list */
	}

	/* Remove a pcb from the free list */
	pcb_PTR p = removeProcQ(&pcbFree_h);
	
	if (pcbFree_h != NULL) 
	{
		/* Initialize the values of the PCB */
		p->p_next = NULL;
		p->p_prev = NULL;
		p->p_prnt = NULL;
		p->p_child = NULL;
		p->p_next_sib = NULL;
		p->p_prev_sib = NULL;
		p->p_semAdd = NULL;
		p->p_time = 0;
		p->p_supportStruct = NULL;
	}
	return p;
}

/****************************************************************************
 * Function: initPcbs
 * Initialize the pcbFree list to contain all the elements of the static 
 * array of MAXPROC pcbs. This method should be called only once during 
 * data structure initialization. 
 */
void initPcbs() {
	pcbFree_h = mkEmptyProcQ();

	int i;
	for(i = 0; i < MAXPROC; i++) {
		freePcb(&(pcbFree_list[i]));
	}
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
void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	if (emptyProcQ(*tp)) {
        /* If queue is empty, insert the first element */
		p -> p_next = p;
		p -> p_prev = p;
	} else {
        /* Insert the new element after the current tail */
		pcb_PTR temp = (*tp);
		p->p_next = temp;
		p->p_prev = temp->p_prev;
		temp->p_prev = p;
		p->p_prev->p_next = p;
	}
	(*tp) = p; /* Update tail pointer */
}

/****************************************************************************
 * Function: removeProcQ
 * Remove the first (i.e. head) element from the process queue whose 
 * tail-pointer is pointed to by tp. 
 * Return NULL if the process queue was initially empty; otherwise return the 
 * pointer to the removed element. Update the process queue’s tail pointer 
 * if necessary. 
 */
pcb_PTR removeProcQ(pcb_PTR *tp) {
	if(emptyProcQ(*tp)) {
		return NULL;
	}
	pcb_PTR tail = *tp;
	if (tail->p_prev == tail) {
		(*tp) = NULL;
		return tail;
	}
	else {
		pcb_PTR remove = tail->p_prev;
		remove->p_prev->p_next = remove->p_next;
		remove->p_next->p_prev = remove->p_prev;
		return remove;
	}
	return NULL;
}

/****************************************************************************
 * Function: outProcQ
 * Remove the pcb pointed to by p from the process queue whose tail-pointer 
 * is pointed to by tp. Update the process queue’s tail pointer if necessary. 
 * If the desired entry is not in the indicated queue (an error condition), 
 * return NULL; otherwise, return p. 
 * Note: p can point to any element of the process queue. 
 */
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p) {
	pcb_PTR tail = (*tp);
	if (emptyProcQ(p)) {
		return NULL;
	}
	if (emptyProcQ(tail)) {
		return NULL;
	}
	pcb_PTR temp = tail->p_next;
	while (temp != p && temp != tail) {
		temp = temp->p_next;
	}
	if (temp == p) {
		p->p_next->p_prev = p->p_prev;
		p->p_prev->p_next = p->p_next;
		if(temp == tail && tail->p_next == tail) {
			(*tp) = NULL;
		}
		else if(temp == tail) {
			(*tp) = tail->p_next;
		}
		return temp;
	} else { 
		return NULL; 
	}
}

/****************************************************************************
 * Function: headProcQ
 * Return the pointer to the first pcb from the process queue whose tail is 
 * pointed to by tp. This pcb will not be removed from the process queue.
 * Return NULL if the process queue is empty.
 */
pcb_PTR headProcQ(pcb_PTR tp) {
	if (emptyProcQ(tp)) {
		return NULL; 
	}
	return tp->p_prev;
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
void insertChild(pcb_PTR prnt, pcb_PTR p) {
	if (!emptyProcQ(p)) {
		if (emptyChild(prnt)) {
			prnt->p_child = p;
			p->p_prnt = prnt;
			p->p_next_sib = NULL;
			p->p_prev_sib = NULL;
		} else {
			p->p_next_sib = prnt->p_child;
			p->p_next_sib->p_prev_sib = p;
			prnt->p_child = p;
		}
	}
}

/****************************************************************************
 * Function: removeChild
 * Make the first child of the pcb pointed to by p no longer a child of p. 
 * Return NULL if initially there were no children of p. 
 * Otherwise, return a pointer to this removed first child pcb.
 */
pcb_PTR removeChild(pcb_PTR p) {
	if (p->p_child == NULL) {
		return NULL;
	}
	pcb_PTR child = p->p_child;
	p->p_child = p->p_child->p_next_sib;

	return child;
}

/****************************************************************************
 * Function: outChild
 * Make the pcb pointed to by p no longer the child of its parent. 
 * If the pcb pointed to by p has no parent, return NULL; 
 * otherwise, return p. 
 * Note: The element pointed to by p need not be the first child of 
 * its parent.
 */
pcb_PTR outChild(pcb_PTR p) {
	if ((p->p_prnt) == NULL) {
		return NULL;
	}
	/* If the process is the first child */
	pcb_PTR prnt = p->p_prnt;
	pcb_PTR currChild = prnt->p_child;
	pcb_PTR lastChild = NULL;

	if(currChild == p) {
		/* If the process is the only child */ 
		return removeChild(prnt);
	}

	while (currChild != NULL) {
		if (currChild == p) {
			lastChild->p_next_sib = p->p_next_sib;
			lastChild->p_prev_sib = p->p_prev_sib;
			return p;
		}
		lastChild = currChild;
		currChild = currChild->p_next_sib;
	}
	return NULL;
}
