#ifndef ASL
#define ASL

/************************** ASL.H ******************************
*
*  The externals declaration file for the Active Semaphore List
*    Module.
*
*  Written by Mikeyg
*/

#include "../h/types.h"

extern int insertBlocked (int *semAdd, pcb_t *p);
extern pcb_t *removeBlocked (int *semAdd);
extern pcb_t *outBlocked (pcb_t *p);
extern pcb_t *headBlocked (int *semAdd);
extern void initASL ();

/***************************************************************/

#endif
