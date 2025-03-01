#ifndef INITIAL_H
#define INITIAL_H

#include "/usr/include/umps3/umps/libumps.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"


extern int processCount;                        /* Number of processes in system */
extern int softBlockCount;                      /* Number of blocked processes */
extern pcb_PTR readyQueue;                      /* Ready queue */
extern pcb_PTR currentProcess;                  /* Currently executing process */
extern int deviceSemaphores[DEVICE_COUNT];    /* Array of device semaphores */
/* add time of day at system start? */

extern void         test();

#endif