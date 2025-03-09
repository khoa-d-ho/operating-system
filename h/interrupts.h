#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/*********************Interrupts.h************************************
*
*        This file declares the externals for interrupts.c
*
*/

#include "../h/types.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/initial.h"
#include "../h/exceptions.h"
#include "/usr/include/umps3/umps/libumps.h"

extern void interruptHandler(state_PTR exceptionsState);
extern void itInterrupt();
extern void pltInterrupt();
extern void nonTimerInterrupt(devregarea_t *devRegA, int lineNo);
/*******************************************************************/

#endif

