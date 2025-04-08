#include "../h/const.h"
#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/exceptions.h"
#include "../h/vmSupport.h"
#include "../h/sysSupport.h"
#include "../h/initProc.h"
#include "/usr/include/umps3/umps/libumps.h"

int p3devSemaphore[DEVICE_COUNT]; 
int masterSemaphore; /* graceful */

void test() {
    static support_t supportStruct[UPROCMAX + 1]; 
    state_t u_procState;   
    int i, j, pid, res;

    masterSemaphore = 0;    
    initSwapStructs(); /* swap pool table and sema4 */
    
    for(j = 0; j < DEVICE_COUNT; j++) {
        p3devSemaphore[j] = 1;
    }

    u_procState.s_pc = (memaddr) TEXTAREASTART;
    u_procState.s_t9 = (memaddr) TEXTAREASTART;
    u_procState.s_status = ALLOFF | PANDOS_IEPBITON | TEBITON | USERPON;
    u_procState.s_sp = (memaddr) STCKTOPEND;
    
    for(pid = 1; pid <= UPROCMAX; pid++) {
        supportStruct[pid].sup_asid = pid;
        u_procState.s_entryHI = (pid << ASIDSHIFT);
        
        /* context for tlb exception handler */
        supportStruct[pid].sup_exceptContext[PGFAULTEXCEPT].c_pc = (memaddr) tlbExceptionHandler;
        supportStruct[pid].sup_exceptContext[PGFAULTEXCEPT].c_stackPtr = (memaddr) &supportStruct[pid].sup_stackTLB[SUPSTCKTOP];
        supportStruct[pid].sup_exceptContext[PGFAULTEXCEPT].c_status = ALLOFF | PANDOS_IEPBITON | PANDOS_CAUSEINTMASK | TEBITON;

        /* context for general exception handler */
        supportStruct[pid].sup_exceptContext[GENERALEXCEPT].c_pc = (memaddr) supLvlGenExceptionHandler;
        supportStruct[pid].sup_exceptContext[GENERALEXCEPT].c_stackPtr = (memaddr) &supportStruct[pid].sup_stackGen[SUPSTCKTOP];
        supportStruct[pid].sup_exceptContext[GENERALEXCEPT].c_status = ALLOFF | PANDOS_IEPBITON | PANDOS_CAUSEINTMASK | TEBITON;

        /* init page table entries */
        for (i = 0; i < PGTBLSIZE; i++) {
            supportStruct[pid].sup_privatePgTbl[i].entryHI = ALLOFF | ((KUSEG + i) << VPNSHIFT) | (pid << ASIDSHIFT);
            supportStruct[pid].sup_privatePgTbl[i].entryLO = ALLOFF | (i << PFNSHIFT) | DIRTYON | VALIDOFF | GLOBALOFF;
        }

        /* stack page */
        supportStruct[pid].sup_privatePgTbl[PGTBLSIZE - 1].entryHI = ALLOFF | (pid << ASIDSHIFT) | (STCKPGVPN << VPNSHIFT);
        supportStruct[pid].sup_privatePgTbl[PGTBLSIZE - 1].entryLO = ALLOFF | DIRTYON | VALIDOFF | GLOBALOFF;
 
        res = SYSCALL(CREATEPROCESS, (unsigned int) &u_procState, (unsigned int) &(supportStruct[pid]), 0);
        
        /* if creation failed, terminate and release master semaphore (review here) */
        if(res != OK) {
            SYSCALL(TERMINATEPROCESS, 0, 0, 0);
            SYSCALL(VERHOGEN, (unsigned int) &masterSemaphore, 0, 0);
        }
    }
    
    for(i = 0; i < UPROCMAX; i++) {
        SYSCALL(PASSEREN, (unsigned int) &masterSemaphore, 0, 0);
    }

    SYSCALL(TERMINATEPROCESS, 0, 0, 0);
}
