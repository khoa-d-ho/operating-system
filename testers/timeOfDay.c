/*	Test of Delay and Get Time of Day */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

void main() {
	unsigned int now1 ,now2;
	int i;

	now1 = SYSCALL(GET_TOD, 0, 0, 0);
	print(WRITETERMINAL, "todTest starts\n");
	now1 = SYSCALL(GET_TOD, 0, 0, 0);

	now2 = SYSCALL(GET_TOD, 0, 0, 0);

	if (now2 < now1)
		print(WRITETERMINAL, "todTest error: time decreasing\n");
	else
		print(WRITETERMINAL, "todTest ok: time increasing\n");

	/* SYSCALL(DELAY, 1, 0, 0);		/* Delay 1 second */
	for (i=0; i < 10000; i++)
		;
	
	now1 = SYSCALL(GET_TOD, 0, 0, 0);

	if ((now1 - now2) < SECOND)
		print(WRITETERMINAL, "todTest error: did not delay one second\n");
	else
		print(WRITETERMINAL, "todTest ok: one second delay\n");
		
	print(WRITETERMINAL, "todTest completed\n");
		
	/* Try to execute nucleys system call. Should cause termination */
	now1 = SYSCALL(GETTIME, 0, 0, 0);
	
	print(WRITETERMINAL, "todTest error: SYS6 did not terminate\n");
	SYSCALL(TERMINATE, 0, 0, 0);
}
