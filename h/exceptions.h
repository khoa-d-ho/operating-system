#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "/usr/include/umps3/umps/libumps.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/initial.h"
#include "../h/interrupts.h"

extern void         exceptionHandler();        
extern void         uTLB_RefillHandler();      
extern void         passUpOrDie(int exceptionType, state_t *exceptionState); 
extern void         getCpuTime(state_t *exceptionState); 
extern void         waitIO(state_t *exceptionState); 
extern void         verhogen(state_t *exceptionState); 
extern void         passeren(state_t *exceptionState); 
extern void         tlbExceptionHandler(state_t *exceptionState); 
extern void         programTrapHandler(state_t *exceptionState); 
extern void         syscallHandler(state_t *exceptionState); 
extern void         copyState(state_t *dest, state_t *src); 

/***************************************************************/

#endif