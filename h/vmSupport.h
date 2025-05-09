#ifndef VMSUPPORT_H
#define VMSUPPORT_H

/**************************************************************************** 
 *
 *  This header file contains the external functions for the virtual memory
 *  support module.
 * 
 ****************************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "../h/deviceSupportDMA.h"
#include "/usr/include/umps3/umps/libumps.h"

extern void toggleInterrupts(int enable);
extern void mutex(int on, int *semAddress);
extern void initSwapStructs();
extern void supTlbExceptionHandler();
extern void markAllFramesFree(int asid);

#endif 
