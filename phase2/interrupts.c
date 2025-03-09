#include "../h/interrupts.h"

void interruptHandler(state_PTR interruptState) {
	int cause = interruptState->s_cause;
	devregarea_t *devrega = (devregarea_t*) RAMBASEADDR;

	if(cause & INTERRUPT_PLT) {
		pltInterrupt();
	}
	if(cause & INTERRUPT_IT) {
		itInterrupt();
	}
	if(cause & INTERRUPT_DISK) {
		nonTimerInterrupt(devrega, DISKINT);
	}
    if(cause & INTERRUPT_FLASH) {
		nonTimerInterrupt(devrega, FLASHINT);
	}
	if(cause & INTERRUPT_NETW) {
		nonTimerInterrupt(devrega, NETWINT);
	}
	if(cause & INTERRUPT_PRINT) {
		nonTimerInterrupt(devrega, PRNTINT);
	}
	if(cause & INTERRUPT_TERM) {
		nonTimerInterrupt(devrega, TERMINT);
	}
}

void nonTimerInterrupt(devregarea_t *devRegA, int lineNo) {
	int devNo;
	
	int devNoMap = devRegA->interrupt_dev[lineNo-3];
	
	if(devNoMap & DEV0) {
		devNo = PROCESSOR; }
	else if(devNoMap & DEV1) {
		devNo = PLTIMER; 
	}
	else if(devNoMap & DEV2) {
		devNo = INTERVALTIMER; 
	}
	else if(devNoMap & DEV3) {
		devNo = DISK; 
	}
	else if(devNoMap & DEV4) {
		devNo = FLASH; 
	}
	else if(devNoMap & DEV5) {
		devNo = NETWORK; 
	}
	else if(devNoMap & DEV6) {
		devNo = PRINTER; 
	}
	else if(devNoMap & DEV7) {
		devNo = TERMINAL; 
	}

	state_PTR exceptionState = (state_PTR) BIOSDATAPAGE;
	
	int devIndex = (lineNo - DISKINT) * DEVPERINT + devNo;
	int statusCode;
	
	if(lineNo == TERMINT)
	{
		statusCode = devRegA->devreg[devIndex].t_transm_status & 0xFF;
		if(statusCode != READY) {
			devRegA->devreg[devIndex].t_transm_command = ACK;
		}
		else{
			statusCode = devRegA->devreg[devIndex].t_recv_status & 0xFF;
			devRegA->devreg[devIndex].t_recv_command = ACK;
			devIndex+= 8;
		}
		
	} else {
		statusCode = devRegA->devreg[devIndex].d_status;
		devRegA->devreg[devIndex].d_command = ACK;
	}
	
	pcb_PTR p;
	deviceSemaphores[devIndex]++;
	if(deviceSemaphores[devIndex] <= 0) {
		p = removeBlocked(&(deviceSemaphores[devIndex]));
		if(p != mkEmptyProcQ()) {
			softBlockCount--;	
			p->p_s.s_v0 = statusCode;
			insertProcQ(&readyQueue, p);
		}
	}
	
	if (currentProcess == mkEmptyProcQ()) {
		scheduler();
	}

	copyState(exceptionState,&(currentProcess->p_s));
	loadNextState(currentProcess->p_s);
}
 
void pltInterrupt() {
	int stopTod;
	STCK(stopTod);
	
	if(currentProcess != mkEmptyProcQ()) {
		copyState((state_PTR)BIOSDATAPAGE, &(currentProcess->p_s));
		currentProcess->p_time += (stopTod - TOD_start);
		insertProcQ(&readyQueue, currentProcess);
		currentProcess = mkEmptyProcQ();
	}
	
	setTIMER(QUANTUM);
	scheduler();
}

void itInterrupt()
{
	LDIT(CLOCKINTERVAL);

	pcb_PTR p = removeBlocked(&(deviceSemaphores[DEVICE_COUNT-1]));
	while(p != mkEmptyProcQ()) {
		insertProcQ(&readyQueue, p);
		softBlockCount--;
		p = removeBlocked(&(deviceSemaphores[DEVICE_COUNT-1]));
	}
	
	(deviceSemaphores[DEVICE_COUNT-1]) = 0;
	
	if(currentProcess == mkEmptyProcQ()) {
		scheduler();
	}
	
	copyState((state_PTR)BIOSDATAPAGE, &(currentProcess->p_s));
	loadNextState(currentProcess->p_s);
}



