#ifndef INTERRUPTS_H
#define INTERRUPTS_H

/************************* INTERRUPTS.H *****************************
*
*  The externals declaration file for the interrupts.
*
*  Written by Khoa Ho & Hieu Tran
*/

#include "../h/types.h"

extern void interruptHandler(); /* Interrupt handler */
extern void pltInterruptHandler(); /* PLT interrupt handler */
extern void deviceInterruptHandler(int line); /* Device interrupt handler */
extern void intervalTimerHandler(); /* Interval timer interrupt handler */
extern void handleDevice(int line, int dev, unsigned int status); /* Handle device interrupts */
extern void handleTerminal(int line, int dev, int isTransmit, unsigned int status); /* Handle terminal device interrupts */
extern int getDevice(int line); /* Get device with pending interrupt on a line */
extern int getHighestPriority(unsigned int cause); /* Identify highest priority pending interrupt */

/***************************************************************/

#endif