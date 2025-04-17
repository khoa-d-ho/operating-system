#ifndef sysSupport_H
#define sysSupport_H

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/vmSupport.h"
#include "/usr/include/umps3/umps/libumps.h"

/* Exception Handlers */
extern void supGeneralExceptionHandler();
extern void supSyscallHandler(support_t *supportPtr);
extern void supProgramTrapHandler();

/* System Calls*/
void terminateUProc(int* sem);
void getTod(state_t *excState);
void writeToPrinter(state_t *excState, int asid);
void writeToTerminal(state_t *excState, int asid);
void readFromTerminal(state_t *excState, int asid);

#endif