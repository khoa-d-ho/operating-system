#ifndef SCHEDULER_H
#define SCHEDULER_H

/************************* SCHEDULER.H *****************************
*
*  The externals declaration file for the scheduler.
*
*  Written by Khoa Ho & Hieu Tran
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/asl.h"
#include "../h/pcb.h"
#include "../h/initial.h"

/* A simple round-robin scheduler*/
extern void scheduler();

extern void loadNextState(state_t state);
extern void copyState(state_t *source, state_t *dest);

/***************************************************************/

#endif