#include "../h/pcb.h"

HIDDEN pcb_t pcbFree_list[MAXPROC];
HIDDEN pcb_PTR pcbFree_h;  

HIDDEN void resetPcb(pcb_PTR p) {
	if (p == NULL)
    	return;
    
    p->p_next = NULL;
    p->p_prev = NULL;
    p->p_prnt = NULL;
    p->p_child = NULL;
    p->p_next_sib = NULL;
    p->p_prev_sib = NULL;
    p->p_s.s_entryHI = 0;
    p->p_s.s_cause = 0;
    p->p_s.s_status = 0;
    p->p_s.s_pc = 0;
    p->p_time = 0;
    p->p_semAdd = NULL; /*  */

    int i;
    for (i = 0; i < STATEREGNUM; i++) {
        p->p_s.s_reg[i] = 0;
    }
}

void freePcb(pcb_PTR p) {
    if (p == NULL) 
        return;
    
    resetPcb(p);

    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

pcb_PTR allocPcb() {
	if (pcbFree_h == NULL)
    	return NULL;

    pcb_PTR p = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;
    
	resetPcb(p);
    
	return p;
}

void initPcbs() {
    pcb_PTR tmp;
    pcbFree_h = &pcbFree_list[0];
    pcbFree_h->p_prev = NULL;
    tmp = pcbFree_h;
    int i;
    for (i = 1;  i < MAXPROC; i++) {
        tmp->p_next = &pcbFree_list[i];
        tmp = tmp->p_next;
        tmp->p_prev = NULL;
    }
    tmp->p_next = NULL;
}

pcb_PTR mkEmptyProcQ() {
	return NULL;
}

int emptyProcQ (pcb_PTR tp) {
    return (tp == NULL);
}

void insertProcQ (pcb_PTR *tp, pcb_PTR p) {
    if (*tp == NULL) {
        *tp = p;
        p->p_next = *tp;
        p->p_prev = *tp;
    }
    else {
        p->p_next = (*tp)->p_next;
        p->p_prev = *tp;
        (*tp)->p_next->p_prev = p;
        (*tp)->p_next = p;
        *tp = p;
    }
}

pcb_PTR removeProcQ (pcb_PTR *tp) {
    if (*tp == NULL) 
        return NULL;
    return outProcQ(tp, (*tp)->p_next);
}


pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p) {
    if (*tp == NULL || p == NULL)
    	return NULL;
    
    /* check p in list */
	if (p->p_prev == NULL || p->p_next == NULL)
    	return NULL;
    
	if (p->p_prev == p && p->p_next == p)
    	*tp = NULL;

    else {
        p->p_next->p_prev = p->p_prev;
    	p->p_prev->p_next = p->p_next;
    	if (*tp == p)
        	*tp = p->p_prev; /* tail removed, new tail is tail.prev */
	} 

    /* dereference the links of p */
	p->p_next = p->p_prev = NULL;
	return p;
}

pcb_PTR headProcQ(pcb_PTR tp) {
	if (tp == NULL)
    	return NULL;
	return tp->p_next;
}

int emptyChild (pcb_PTR p) {
	return p->p_child == NULL;
}

void insertChild (pcb_PTR prnt, pcb_PTR p) {
    insertProcQ(&(prnt->p_child), p);
	p->p_prnt = prnt;
}


pcb_PTR removeChild (pcb_PTR p) {
	return removeProcQ(&(p->p_child));
}

pcb_PTR outChild (pcb_PTR p) {
    pcb_PTR prnt = p->p_prnt;
	if (prnt == NULL) 
    	return NULL;
	return outProcQ(&(prnt->p_child), p);
}
