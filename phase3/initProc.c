#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/exceptions.h"
#include "../h/vmSupport.h"
#include "../h/sysSupport.h"
#include "../h/initProc.h"
#include "/usr/include/umps3/umps/libumps.h"

int devSemaphore[DEVICE_COUNT-1]; 
int masterSemaphore; 
HIDDEN void createUProc(int id);
HIDDEN support_t supStructs[UPROCMAX+1];

void debug8 (int a, int b, int c, int d) {
    int i;
    i = 0;
    i++;
}

void test() {
	int i;
	for(i=0; i < (DEVICE_COUNT-1); i++) {
		devSemaphore[i] = 1;
	}	

	initSwapStructs();

	int id;
	for(id=1; id <= UPROCMAX; id++) {
		createUProc(id);
	}
    
	masterSemaphore = 0;
	for(i=0; i < UPROCMAX; i++) {
		SYSCALL(PASSEREN, (int) &masterSemaphore, 0, 0);
	}
    

	SYSCALL(TERMPROCESS, 0, 0, 0);

} 

void createUProc(int id) {
    state_t newState;
    newState.s_pc = (memaddr) TEXTAREAADDR;
    newState.s_t9 = (memaddr) TEXTAREAADDR;
    newState.s_sp = (memaddr) STACKPAGEADDR;
    newState.s_status = ALLOFF | TEBITON | IMON | IEPON | UMON;

    supStructs[id].sup_asid = id;
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) supGeneralExceptionHandler;
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr) &(supStructs[id].sup_stackGen[499]);
    supStructs[id].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | IEPON | IMON | TEBITON;
        
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) supTlbExceptionHandler;
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr) &(supStructs[id].sup_stackTLB[499]);
    supStructs[id].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | IEPON | IMON | TEBITON;
    int pg;
    for(pg=0; pg < MAXPAGES; pg++) {
        supStructs[id].sup_privatePgTbl[pg].entryHI = ALLOFF | ((UPROCSTART + pg) << VPNSHIFT) | (id << ASIDSHIFT);
        supStructs[id].sup_privatePgTbl[pg].entryLO = ALLOFF | DIRTYON;
    }

    newState.s_entryHI = KUSEG | (id << ASIDSHIFT) | ALLOFF;
    supStructs[id].sup_privatePgTbl[MAXPAGES-1].entryHI = ALLOFF | (PAGESTACK << VPNSHIFT) | (id << ASIDSHIFT);

    debug8(0, 0, 0, 0);
    int status = SYSCALL(CREATEPROCESS, (int) &(newState), (int) &(supStructs[id]), 0);
        
    if (status != 0) {
        SYSCALL(TERMPROCESS, 0, 0, 0);
    }
}
