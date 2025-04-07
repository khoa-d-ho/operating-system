#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "../h/const.h"
#include "../h/types.h"

/* swap pool table structure */
typedef struct {
    int asid;                
    int pageNo;             
    pteEntry_t *ptePtr;      
} swap_t;

/* static data */
/* helper */
HIDDEN swap_t swapPool[POOLSIZE];
HIDDEN int swapPoolSem;      
HIDDEN int nextVictim = 0;   

HIDDEN int pickVictim();     
HIDDEN int flashIO(int operation, int devNo, int blockNo, int frameAddr);

void initSwapStructs();
void uTLB_RefillHandler();
void tlbExceptionHandler();

#endif 
