/* Tests the memory management. */

#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"


void main () {
	char i;
	int corrupt;

	print(WRITETERMINAL, "swapTest starts\n");

	/* write into the first word of pages 20-29 of kuseg */
	for (i = 20; i < 30; i++) {
		*(int *)(SEG2 + (i * PAGESIZE)) = i;
		/* print(WRITETERMINAL, "swapTest ok: wrote to page of VM\n"); */
	}

	print(WRITETERMINAL, "swapTest ok: wrote to pages of seg kuseg\n");

	/* check if first word of pages still contain what we wrote */
	corrupt = FALSE;
	for (i = 20; i < 30; i++)
		if (*(int *)(SEG2 + i * PAGESIZE) != i) {
			print(WRITETERMINAL, "swapTest error: swapper corrupted data\n");
			corrupt = TRUE;
			break;
		}

	if (corrupt == FALSE)
		print(WRITETERMINAL, "swapTest ok: data survived swapper\n");
	
	/* try to access segment ksegOS Should cause termination */
	/* i = getSTATUS(); */
	i = *((char *)(0x20000000));
	print(WRITETERMINAL, "swapTest error: could access segment kseg1\n");
	
	SYSCALL(TERMINATE, 0, 0, 0);
}
