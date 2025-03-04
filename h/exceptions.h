
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "types.h"
#include "const.h"
#include "pcb.h"

/* System Call Codes */
#define CREATEPROCESS    1
#define TERMPROCESS      2
#define PASSEREN         3
#define VERHOGEN         4
#define WAITFORIO        5
#define GETCPUT          6
#define WAITFORCLOCK     7
#define GETSUPPORTT      8


/* Main Exception Handler */
void exceptionHandler();

/* Specific Exception Handlers */
void syscallHandler(state_t *excState);
void tlbExceptionHandler();
void programTrapHandler();

/* System Calls */
void createProcess();
void terminateProcess(pcb_PTR current);
void passeren(int *semAddress);
pcb_PTR verhogen(int *semAddress);
void waitIO();
void getCpuTime();
void waitClock();
void getSupportData();

/* Support Functions */
void copyState(state_t *source, state_t *dest);
void passUpOrDie(unsigned int passUpCode);
void loadNextState(state_t state);

#endif /* EXCEPTIONS_H */