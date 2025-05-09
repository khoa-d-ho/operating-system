#ifndef DEVICESUPPORTDMA_H
#define DEVICESUPPORTDMA_H

/**************************************************************************** 
 * 
 *  This header file contains the external functions for the device support
 *  module.
 * 
 ****************************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/initProc.h"
#include "../h/sysSupport.h"
#include "/usr/include/umps3/umps/libumps.h"
#include "../h/vmSupport.h"

int flashOperation(int operation, int devNo, int blockNo, int frameAddr);
int diskOperation(int operation, int devNo, int sectorNo, int frameAddr);

#endif