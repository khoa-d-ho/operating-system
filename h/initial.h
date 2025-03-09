#ifndef INITIAL_H
#define INITIAL_H

#include "/usr/include/umps3/umps/libumps.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/exceptions.h"

extern void test();
extern void uTLB_RefillHandler();

extern int processCount;            
extern int softBlockCount;                 
extern pcb_PTR readyQueue;               
extern pcb_PTR currentProcess;          
extern int deviceSemaphores[DEVICE_COUNT];  
extern cpu_t TOD_start;

#endif