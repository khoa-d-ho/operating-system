#include "../h/asl.h"
#include "../h/pcb.h"

HIDDEN semd_t *semd_h; /* Head of the active semaphore list */ 
HIDDEN semd_t *semdFree_h; /* Head of the free semaphore list */
HIDDEN semd_t semdTable[MAXPROC+2]; /* Array of semaphores */

HIDDEN semd_t *traverseASL(int *semAdd, semd_t **prev) {
    *prev = semd_h;  /* Start with the first dummy node */
    semd_t *curr = semd_h->s_next; /* Skip the first dummy node */
    while (curr != NULL && curr->s_semAdd < semAdd) {
        *prev = curr;
        curr = curr->s_next;
    }
    return curr;
}

int insertBlocked(int *semAdd, pcb_PTR p) {
    semd_t *prev;
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd) {
        if (semdFree_h == NULL) {
            return TRUE;
        }
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

pcb_PTR removeBlocked(int *semAdd) {
    semd_t *prev;
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd) {
        return NULL;
    }
    pcb_PTR p = removeProcQ(&(sem->s_procQ));
    if (emptyProcQ(sem->s_procQ)) {
        prev->s_next = sem->s_next;
        sem->s_next = semdFree_h;
        semdFree_h = sem;
    }
    return p;
}

pcb_PTR outBlocked(pcb_PTR p) {
    semd_t *prev = semd_h;
    semd_t *sem = semd_h->s_next;
    while (sem != NULL) {
        pcb_PTR removed = outProcQ(&(sem->s_procQ), p);
        if (removed != NULL) {
            if (emptyProcQ(sem->s_procQ)) {
                prev->s_next = sem->s_next;
                sem->s_next = semdFree_h;
                semdFree_h = sem;
            }
            return removed;
        }
        prev = sem;
        sem = sem->s_next;
    }
    return NULL;
}

pcb_PTR headBlocked(int *semAdd) {
    semd_t *prev;
    semd_t *sem = traverseASL(semAdd, &prev);
    if (sem == NULL || sem->s_semAdd != semAdd || emptyProcQ(sem->s_procQ)) {
        return NULL;
    }
    return headProcQ(sem->s_procQ);
}

void initASL() {
    semdFree_h = &semdTable[1];

    semd_h = &(semdTable[0]);
    semd_h->s_semAdd = (int *)(0x00000000); /*  value 0 for the first dummy node */
    semd_h->s_procQ = NULL;

    semd_h->s_next = &(semdTable[MAXPROC+1]);
    semd_h->s_next->s_procQ = NULL;
    semd_h->s_next->s_next = NULL;
    semd_h->s_next->s_semAdd = (int *)MAXINT; /* value MAXINT for the last dummy node */

    semd_t *tmp = semdFree_h;
    int i;
    for (i = 2; i < MAXPROC+1; i++) {
        tmp->s_next = &(semdTable[i]);
        tmp->s_procQ = NULL;
        tmp = tmp->s_next;
    }
    tmp->s_procQ = NULL;
    tmp->s_next = NULL;
}