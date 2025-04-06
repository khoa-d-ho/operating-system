#ifndef TCONST
#define TCONST

/************************** TCONST.H ******************************
*
*  Written by Mikeyg
*/

#define FALSE			0
#define TRUE			1
#define PAGESIZE		4096
#define SECOND			1000000
#define EOS				'\0'
#define PAGESIZE		4096
#define WORDLEN			4
#define READY			1


#define	GETTIME			6

/* level 1 SYS calls */
#define TERMINATE		9
#define GET_TOD			10
#define WRITEPRINTER	11
#define WRITETERMINAL 	12
#define READTERMINAL	13
#define DISK_GET		14
#define DISK_PUT		15
#define	FLASH_GET		16
#define FLASH_PUT		17
#define DELAY			18
#define PSEMVIRT		19
#define VSEMVIRT		20

#define SEG0			0x00000000
#define SEG1			0x40000000
#define SEG2			0x80000000
#define SEG3			0xC0000000

/***************************************************************/

#endif
