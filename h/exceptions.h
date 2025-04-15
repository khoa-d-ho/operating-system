
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

/********************* EXCEPTIONS.H ************************************
*
*        This file declares the externals for exceptions.c
*
*/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "/usr/include/umps3/umps/libumps.h"

/* Main Exception Handler */
void exceptionHandler();

/* Specific Exception Handlers */
extern void syscallHandler();
extern void tlbExceptionHandler();
extern void programTrapHandler();
extern void uTLB_RefillHandler();

/* System Calls */
void createProcess();
void terminateProcess(pcb_PTR current);
void passeren();
void verhogen();
void waitIO();
void getCPUTime();
void waitClock();
void getSupportData();

/* Support Functions */
void copyState(state_t *source, state_t *dest);
void passUpOrDie(int passUpCode);

#endif

