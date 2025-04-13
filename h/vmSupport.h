#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
/* #include "../h/sysSupport.h" */
#include "/usr/include/umps3/umps/libumps.h"

/* static data */
/* helper */
HIDDEN swap_t swapPool[POOLSIZE];
HIDDEN int swapPoolSem;      
HIDDEN int nextVictim = 0;   

HIDDEN int pickVictim();     
HIDDEN int flashIO(int operation, int devNo, int blockNo, int frameAddr);
HIDDEN void toggleInterrupts(int enable);
HIDDEN void mutex(int state, int *semAddress);

extern void initSwapStructs();
extern void tlbExceptionHandler();

#endif 
