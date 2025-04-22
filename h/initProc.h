#ifndef INITPROC_H
#define INITPROC_H

/**************************************************************************** 
 *
 *  This header file contains the externals for the process
 *  initialization module.
 * 
 ****************************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/sysSupport.h"
#include "../h/vmSupport.h"
#include "../h/initial.h"
#include "../h/pcb.h"
#include "../h/exceptions.h"
#include "/usr/include/umps3/umps/libumps.h"

extern int devSemaphore[DEVICE_COUNT-1]; 
extern int masterSemaphore;     

void test();

#endif 
