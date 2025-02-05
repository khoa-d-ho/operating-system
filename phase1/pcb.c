#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"

#define MAXPROC	20
#define	MAXSEM	MAXPROC

static pcb_t pcbFree_list[MAXPROC];
HIDDEN pcb_t *pcbFree_h;  

HIDDEN void resetPcb(pcb_t *p) {
	if (p == NULL)
    	return;
    
    p->p_next = NULL;
    p->p_prev = NULL;
    p->p_prnt = NULL;
    p->p_child = NULL;
    p->p_sib = NULL;
    p->p_s.s_entryHI = 0;
    p->p_s.s_cause = 0;
    p->p_s.s_status = 0;
    p->p_s.s_pc = 0;
    p->p_time = 0;
    p->p_semAdd = NULL; //

    for (int i = 0; i < STATEREGNUM; i++) {
        p->p_s.s_reg[i] = 0;
    }
}

void freePcb(pcb_t *p) {
    if (p == NULL) {
        return;
    }

    resetPcb(p);
    
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}

pcb_t *allocPcb() {
	if (pcbFree_h == NULL)
    	return NULL;
	pcb_t *p = removeProcQ(&pcbFree_h); //
    
	resetPcb(p);
    
	return p;
}

void initPcbs() {
	pcbFree_h = &(pcbFree_list[0]); //
	for (int i = 0; i < MAXPROC; i++) {
    	resetPcb(&pcbFree_list[i]);
    	insertProcQ(&pcbFree_h, &pcbFree_list[i]);
	}
}

pcb_t *mkEmptyProcQ() {
	return NULL;
}

int emptyProcQ (pcb_t *tp) {
    return (tp == NULL);
}

void insertProcQ (pcb_t **tp, pcb_t *p) {
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

pcb_t *removeProcQ (pcb_t **tp) {
    
}