#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"

HIDDEN pcb_t *pcbFree_h;  

/* Process Control Block free list */
void freePcb(pcb_t *p) {
    if (p == NULL) {
        return;
    }
    
    /* Add to the pcbFree list */
    p->p_next = pcbFree_h;
    pcbFree_h = p;
}
