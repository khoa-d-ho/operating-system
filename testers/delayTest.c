/*	Test of Delay and Get Time of Day */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

void main() {
	unsigned int now1 ,now2;

	now1 = SYSCALL(GET_TOD, 0, 0, 0);
	print(WRITETERMINAL, "Delay test starts\n");
	now1 = SYSCALL(GET_TOD, 0, 0, 0);

	now2 = SYSCALL(GET_TOD, 0, 0, 0);

	if (now2 < now1)
		print(WRITETERMINAL, "Delay Test error: time decreasing\n");
	else
		print(WRITETERMINAL, "Delay Test ok: time increasing\n");

	SYSCALL(DELAY, 2, 0, 0);		/* Delay 2 second */
	now1 = SYSCALL(GET_TOD, 0, 0, 0);

	if ((now1 - now2) < SECOND)
		print(WRITETERMINAL, "Delay Test error: did not delay one second\n");
	else
		print(WRITETERMINAL, "Delay Test ok: two second delay\n");
		
	print(WRITETERMINAL, "Delay Test completed\n");
		
	/* Try to execute nucleys system call. Should cause termination */
	now1 = SYSCALL(GETTIME, 0, 0, 0);
	
	print(WRITETERMINAL, "todTest error: SYS6 did not terminate\n");
	SYSCALL(TERMINATE, 0, 0, 0);
}
