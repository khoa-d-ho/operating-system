/*	Test Disk Get and Disk Put */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

#define MILLION	1000000

void main() {
	/*char buffer[PAGESIZE]; */
	int i;
	int dstatus;
	int *buffer;
	
	buffer = (int *)(SEG2 + (20 * PAGESIZE));

	print(WRITETERMINAL, "diskTest starts\n");
	*buffer = 42;  /*buffer[0] = 'a'; */
	dstatus = SYSCALL(DISK_PUT, (int)buffer, 1, 3);
	
	if (dstatus != READY)
		print(WRITETERMINAL, "diskTest error: disk i/o result\n");
	else
		print(WRITETERMINAL, "diskTest ok: disk i/o result\n");

	*buffer = 100;  /*buffer[0] = 'z'; */
	dstatus = SYSCALL(DISK_PUT, (int)buffer, 1, 23);

	dstatus = SYSCALL(DISK_GET, (int)buffer, 1, 3);
	
	if (*buffer != 42)  /*(buffer[0] != 'a') */
		print(WRITETERMINAL, "diskTest error: bad first disk sector readback\n");
	else
		print(WRITETERMINAL, "diskTest ok: first disk sector readback\n");

	dstatus = SYSCALL(DISK_GET, (int)buffer, 1, 23);
	
	if (*buffer != 100) /*(buffer[0] != 'z') */
		print(WRITETERMINAL, "diskTest error: bad second disk sector readback\n");
	else
		print(WRITETERMINAL, "diskTest ok: second disk sector readback\n");

	/* should eventually exceed device capacity */
	i = 0;
	dstatus = SYSCALL(DISK_GET, (int)buffer, 1, i);
	while ((dstatus == READY) && (i < MILLION)) {
		i++;
		dstatus = SYSCALL(DISK_GET, (int)buffer, 1, i);
	}
	
	if (i < MILLION)
		print(WRITETERMINAL, "diskTest ok: device capacity detection\n");
	else
		print(WRITETERMINAL, "diskTest error: device capacity undetected\n");
		
	print(WRITETERMINAL, "diskTest: completed\n");

	/* try to do a disk read into protected RAM */
	SYSCALL(DISK_GET, SEG1, 1, 3);

	print(WRITETERMINAL, "diskTest error: just read into segment 1\n");

	/* generate a variety of program traps */
	i = i / 0;
	print(WRITETERMINAL, "diskTest error: divide by 0 did not terminate\n");

	LDST(buffer);
	print(WRITETERMINAL, "diskTest error: priv. instruction did not terminate\n");

	SYSCALL(1, (int) buffer, 0, 0);
	print(WRITETERMINAL, "diskTest error: sys1 did not terminate\n");
	
	SYSCALL(TERMINATE, 0, 0, 0);
}
