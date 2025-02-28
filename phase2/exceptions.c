#include "../h/exceptions.h"

void tlbExceptionHandler(state_t *exceptionState);
void programTrapHandler(state_t *exceptionState);
void syscallHandler(state_t *exceptionState);
void interruptHandler();
void uTLB_RefillHandler();
void passUpOrDie(int exceptionType, state_t *exceptionState);
void getCpuTime(state_t *exceptionState);
void waitIO(state_t *exceptionState);
void verhogen(state_t *exceptionState);
void passeren(state_t *exceptionState);
void createProcess(state_t *exceptionState);
void terminateProcess(pcb_PTR process);
void getSupportPtr(state_t *exceptionState);
void waitClock(state_t *exceptionState);

void exceptionHandler() {    
    switch (EXCEPTION_CODE) {
        /* code 0 */
        case(INTERRUPTS):
            interruptHandler();
            break;
        /* code 1-3 */
        case(TLBMOD): 
        case(TLBINVLD):
        case(TLBINVLDL):
            tlbExceptionHandler(PGFAULTEXCEPT);
            break;

        /* code 4-7, 9-12 */
        case(ADDRINVLD):
        case(ADDRINVLDS):
        case(BUSINVLD):
        case(BUSINVLDL):
        case(BREAKPOINT):
        case(RESERVEDINST):
        case(COPROCUNUSABLE):
        case(ARITHOVERFLOW):
            syscallHandler(GENERALEXCEPT);
            break;

        /* code 8 */
        case(SYSCALL_EXCEPTION):
            programTrapHandler();
            break;
    }
}


void uTLB_RefillHandler () {
    setENTRYHI(KUSEG);
    setENTRYLO(KSEG0);
    TLBWR();
    LDST ((state_PTR) BIOSDATAPAGE);
}