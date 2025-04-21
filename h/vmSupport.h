#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "/usr/include/umps3/umps/libumps.h"

/* helper */
extern swap_t swapPool[POOLSIZE];

extern void toggleInterrupts(int enable);
extern void mutex(int state, int *semAddress);
extern void initSwapStructs();
extern void supTlbExceptionHandler();

#endif 
