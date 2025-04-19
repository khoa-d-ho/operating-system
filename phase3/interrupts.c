#include "../h/interrupts.h"

/****************************************************************************
 * interrupts.c
 * 
 * This file contains the implementation of the interrupt handling for the 
 * operating system. It handles the different types of interrupts and calls the
 * appropriate interrupt handler. In particular,
 * - pltInterrupt() handles the PLT timer interrupt
 * - itInterrupt() handles the interval timer interrupt
 * - nonTimerInterrupt() handles the non-timer interrupts, of which there are 5
 * (disk, flash, network, printer, terminal). Terminal interrupts are further 
 * divided into transmit and receive interrupts.
 * 
 * Helper functions include:
 * - getDeviceNumber() returns the device number given the device bitmask of the
 * interrupting device.
 * - ackTermInterrupt() acknowledges the terminal interrupt and returns the
 * status code.
 * - ackDeviceInterrupt() acknowledges the non-terminal interrupt and returns the
 * status code.
 * - handleDevSemaphore() handles the device semaphore and wakes up the blocked
 * process if necessary.
 * 
 * Written by Khoa Ho & Hieu Tran
 * February 2025
 ****************************************************************************/

/*****************************************************************************
 * Function: getDeviceNumber
 * 
 * This function returns the device number given the device bitmask. The device
 * bitmask is used to identify the device that caused the interrupt.
 * Parameters:
 *   devBitmask - The device bitmask.
 * Returns:
 *   The device number. If the device is not found, it returns -1.
 */
HIDDEN int getDeviceNumber(int devBitmask) {
    if (devBitmask & DEV0) {
        return PROCESSOR;
    } else if (devBitmask & DEV1) {
        return PLTIMER;
    } else if (devBitmask & DEV2) {
        return INTERVALTIMER;
    } else if (devBitmask & DEV3) {
        return DISK;
    } else if (devBitmask & DEV4) {
        return FLASH;
    } else if (devBitmask & DEV5) {
        return NETWORK;
    } else if (devBitmask & DEV6) {
        return PRINTER;
    } else if (devBitmask & DEV7) {
        return TERMINAL;
    }
    return -1; /* device not found */ 
}

/*****************************************************************************
 * Function: ackTermInterrupt
 * 
 * This function acknowledges the terminal interrupt and returns the status code.
 * It checks the transmit status first, and if it is not ready, it acknowledges
 * the transmit command. If the transmit status is ready, it checks the receive
 * status and acknowledges the receive command.
 * Parameters:
 *   devRegA - The device register area.
 *   devIndex - The device index.
 * Returns:
 *   The status code of the terminal interrupt.
 */
HIDDEN int ackTermInterrupt(devregarea_t *devRegA, int devIndex) {
	/* Get status code by masking the status register to the right 8 bits */
    int statusCode = devRegA->devreg[devIndex].t_transm_status & BITMASK_8;
    if (statusCode != READY) {
		/* If the transmit status is not ready, acknowledge the transmit command */
        devRegA->devreg[devIndex].t_transm_command = ACK;
    } else {
		/* If the transmit status is ready, acknowledge the receive command */
        statusCode = devRegA->devreg[devIndex].t_recv_status & BITMASK_8;
        devRegA->devreg[devIndex].t_recv_command = ACK;
        devIndex += DEVPERINT;
    }
    return statusCode;
}

/*****************************************************************************
 * Function: ackDeviceInterrupt
 * 
 * This function acknowledges the non-terminal interrupt and returns the status
 * code. It sets the command register to ACK and returns the status code.
 * Parameters:
 *   devRegA - The device register area.
 *   devIndex - The device index.
 * Returns:
 *   The status code of the non-terminal interrupt.
 */
HIDDEN int ackDeviceInterrupt(devregarea_t *devRegA, int devIndex) {
    int statusCode = devRegA->devreg[devIndex].d_status;
    devRegA->devreg[devIndex].d_command = ACK;
    return statusCode;
}

/*****************************************************************************
 * Function: handleDevSemaphore
 * 
 * This function handles the device semaphore and wakes up the blocked process
 * if necessary. It increments the device semaphore and checks
 * if there are any blocked processes. If there are, it removes the blocked
 * process from the blocked queue and inserts it into the ready queue.
 * Parameters:
 *   devIndex - The device index.
 *   statusCode - The status code of the interrupt.
 */
HIDDEN void handleDevSemaphore(int devIndex, int statusCode) {
    pcb_PTR p;
    deviceSemaphores[devIndex]++;
    if (deviceSemaphores[devIndex] <= 0) {
		/* If there are blocked processes, remove the first one 
			and insert it into the ready queue */
        p = removeBlocked(&(deviceSemaphores[devIndex]));
        if (p != mkEmptyProcQ()) {
			/* If process is not null, decrement the soft block count
				and set the status code  */ 
            softBlockCount--;
            p->p_s.s_v0 = statusCode;
            insertProcQ(&readyQueue, p);
        }
    }
}

void interruptHandler(state_PTR interruptState) {
	/* Get the cause of the interrupt */
	int cause = interruptState->s_cause;

	/* Get the device register area to find the device that caused the interrupt */
	devregarea_t *devrega = (devregarea_t*) RAMBASEADDR;

	if (cause & INTERRUPT_PLT) {
		pltInterrupt();
	}
	if (cause & INTERRUPT_IT) {
		itInterrupt();
	}
	if (cause & INTERRUPT_DISK) {
		nonTimerInterrupt(devrega, DISKINT);
	}
    if (cause & INTERRUPT_FLASH) {
		nonTimerInterrupt(devrega, FLASHINT);
	}
	if (cause & INTERRUPT_NETW) {
		nonTimerInterrupt(devrega, NETWINT);
	}
	if (cause & INTERRUPT_PRINT) {
		nonTimerInterrupt(devrega, PRNTINT);
	}
	if (cause & INTERRUPT_TERM) {
		nonTimerInterrupt(devrega, TERMINT);
	}
}

/*****************************************************************************
 * Function: nonTimerInterrupt
 * 
 * This function handles the non-timer interrupts. It checks the line number of
 * the interrupt and calls the appropriate function to handle the interrupt.
 * It also handles the terminal interrupts by checking the transmit and receive
 * status.
 * Parameters:
 *   devRegA - The device register area.
 *   lineNo - The line number of the interrupt.
 */
void nonTimerInterrupt(devregarea_t *devRegA, int lineNo) {
    int devBitmask = devRegA->interrupt_dev[lineNo - 3];
    int devNo = getDeviceNumber(devBitmask);
    state_PTR exceptionState = (state_PTR) BIOSDATAPAGE;

	/* Get the device index of the interrupt */
    int devIndex = (lineNo - DISKINT) * DEVPERINT + devNo;
    int statusCode;

    if (lineNo == TERMINT) {
        statusCode = ackTermInterrupt(devRegA, devIndex);
    } else {
        statusCode = ackDeviceInterrupt(devRegA, devIndex);
    }

    handleDevSemaphore(devIndex, statusCode);

    if (currentProcess == mkEmptyProcQ()) {
		/* If there is no current process, set the timer and scheduler */
        scheduler();
    }

	exceptionState->s_cause &= ~(
		(lineNo==DISKINT)  ? INTERRUPT_DISK   :
		(lineNo==FLASHINT) ? INTERRUPT_FLASH  :
		(lineNo==NETWINT)  ? INTERRUPT_NETW   :
		(lineNo==PRNTINT)  ? INTERRUPT_PRINT  :
		(lineNo==TERMINT)  ? INTERRUPT_TERM   : 0
  );

	/* Copy the state of the current process to the exception state */
    copyState(exceptionState, &(currentProcess->p_s));
    loadNextState(currentProcess->p_s);
}
 
/*****************************************************************************
 * Function: pltInterrupt
 * 
 * This function handles the PLT timer interrupt. It checks if there is a
 * current process and if so, it copies the state of the current process to the
 * BIOS data page and inserts it into the ready queue. It then sets the timer
 * and calls the scheduler.
 */
void pltInterrupt() {
	/* Get the current time (when the interrupt occurred) */
	state_PTR exceptionState = (state_PTR) BIOSDATAPAGE;

	int stopTod;
	STCK(stopTod);
	
	if(currentProcess != mkEmptyProcQ()) {
		/* If there is a current process, copy the state of the current process 
			to the BIOS data page and insert it into the ready queue */
		copyState((state_PTR)BIOSDATAPAGE, &(currentProcess->p_s));
		currentProcess->p_time += (stopTod - TOD_start);
		insertProcQ(&readyQueue, currentProcess);
		currentProcess = mkEmptyProcQ();
	}

	exceptionState->s_cause &= ~INTERRUPT_PLT;
	/* Acknowledge the interrupt and call the scheduler */
	setTIMER(QUANTUM);
	scheduler();
}

/*****************************************************************************
 * Function: itInterrupt
 * 
 * This function handles the interval timer interrupt. It checks if there is a
 * current process and if so, it copies the state of the current process to the
 * BIOS data page and inserts it into the ready queue. It then sets the timer
 * and calls the scheduler.
 */
void itInterrupt() {
	/* Acknowledge the interrupt by loading the interval timer */
	state_PTR exceptionState = (state_PTR) BIOSDATAPAGE;

	LDIT(CLOCKINTERVAL);

	/* Get the first process in the blocked queue */
	pcb_PTR p = removeBlocked(&(deviceSemaphores[CLOCK])); 
	while (p != mkEmptyProcQ()) {
		/* Unblock all pcbs blocked on the Pseudo-clock semaphore. */
		insertProcQ(&readyQueue, p);
		softBlockCount--;
		p = removeBlocked(&(deviceSemaphores[CLOCK]));
	}
	
	/* Reset the Pseudo-clock semaphore to zero */
	(deviceSemaphores[CLOCK]) = 0;
	
	if(currentProcess == mkEmptyProcQ()) {
		scheduler();
	}

	exceptionState->s_cause &= ~INTERRUPT_IT;
	copyState((state_PTR)BIOSDATAPAGE, &(currentProcess->p_s));
	loadNextState(currentProcess->p_s);
}
