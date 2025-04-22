#ifndef sysSupport_H
#define sysSupport_H

/**************************************************************************** 
 *
 *  This header file contains the external functions for the system support
 *  module.
 * 
 ****************************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/vmSupport.h"
#include "/usr/include/umps3/umps/libumps.h"

/* Exception Handlers */
extern void supGeneralExceptionHandler();
extern void supSyscallHandler(support_t *supportPtr);
extern void supProgramTrapHandler();

#endif