#ifndef INITPROC_H
#define INITPROC_H

#include "../h/const.h"
#include "../h/types.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/initial.h"

extern int devSemaphore[DEVICE_COUNT-1]; 
extern int masterSemaphore;      


void test();
#endif 
