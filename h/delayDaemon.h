#ifndef DELAYDAEMON_H
#define DELAYDAEMON_H

/********************* DELAYDAEMON.H *******************************
*
*        This file declares the externals for delayDaemon.c
*
*******************************************************************/

#include "../h/const.h"
#include "../h/types.h"
#include "../h/asl.h"
#include "../h/pcb.h"
#include "../h/initial.h"
#include "../h/scheduler.h"
#include "../h/exceptions.h"
#include "../h/interrupts.h"
#include "../h/initProc.h"
#include "../h/vmSupport.h"
#include "../h/sysSupport.h"

void initADL();

void delayFacility();

void delayDaemon();

#endif