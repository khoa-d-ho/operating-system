#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/************************* INTERRUPTS.H *****************************
*
*  The externals declaration file for the interrupt handlers.
*
*  Written by Khoa Ho & Hieu Tran
*/

#include "../h/types.h"

/* Main interrupt handler */
extern void interruptHandler();

/* Timer interrupt handlers */
extern void pltInterruptHandler();
extern void intervalTimerHandler();

/* Device interrupt handlers */
extern void deviceInterruptHandler(int line);
extern void handleTerminal(int line, int dev, int isTransmit, unsigned int status);
extern void handleDevice(int line, int dev, unsigned int status);

/***************************************************************/

#endif