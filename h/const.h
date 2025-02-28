#ifndef CONSTS
#define CONSTS

/**************************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 ****************************************************************************/

/* Hardware & software constants */
#define PAGESIZE		  4096			/* page size in bytes	*/
#define WORDLEN			  4				/* word size in bytes	*/
#define MAXPROC			  20			/* max number of processes */


/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR		0x10000000
#define RAMBASESIZE		0x10000004
#define TODLOADDR		  0x1000001C
#define INTERVALTMR		0x10000020	
#define TIMESCALEADDR	0x10000024


/* utility constants */
#define	TRUE			    1
#define	FALSE			    0
#define HIDDEN			  static
#define EOS				    '\0'
#define MAXINT       0x7FFFFFFF

#define NULL 			    ((void *)0xFFFFFFFF)

/* device interrupts */
#define DISKINT			  3
#define FLASHINT 		  4
#define NETWINT 		  5
#define PRNTINT 		  6
#define TERMINT			  7

#define DEVINTNUM		  5		  /* interrupt lines used by devices */
#define DEVPERINT		  8		  /* devices per interrupt line */
#define DEVREGLEN		  4		  /* device register field length in bytes, and regs per dev */	
#define DEVREGSIZE	  16 		/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS			  0
#define COMMAND			  1
#define DATA0			    2
#define DATA1			    3

/* device register field number for terminal devices */
#define RECVSTATUS  	0
#define RECVCOMMAND 	1
#define TRANSTATUS  	2
#define TRANCOMMAND 	3

/* device common STATUS codes */
#define UNINSTALLED		0
#define READY			    1
#define BUSY			    3

/* device common COMMAND codes */
#define RESET			    0
#define ACK				    1

/* Memory related constants */
#define KSEG0           0x00000000
#define KSEG1           0x20000000
#define KSEG2           0x40000000
#define KUSEG           0x80000000
#define RAMSTART        0x20000000
#define BIOSDATAPAGE    0x0FFFF000
#define	PASSUPVECTOR	0x0FFFF900

/* Exceptions related constants */
#define	PGFAULTEXCEPT	  0
#define GENERALEXCEPT	  1


/* operations */
#define	MIN(A,B)		((A) < (B) ? A : B)
#define MAX(A,B)		((A) < (B) ? B : A)
#define	ALIGNED(A)		(((unsigned)A & 0x3) == 0)

/* Macro to load the Interval Timer */
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

/* Macro to read the TOD clock */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))

/* Timing constants */
#define QUANTUM         5000      /* 5ms in microseconds */
#define CLOCKINTERVAL   100000    /* 100ms in microseconds */

/* system call codes */
#define	CREATETHREAD	1	/* create thread */
#define	TERMINATETHREAD	2	/* terminate thread */
#define	PASSERN			3	/* P a semaphore */
#define	VERHOGEN		4	/* V a semaphore */
#define	WAITIO			5	/* delay on a io semaphore */
#define	GETCPUTIME		6	/* get cpu time used to date */
#define	WAITCLOCK		7	/* delay on the clock semaphore */
#define	GETSPTPTR		8	/* return support structure ptr. */

/* Device Constants */
#define DEVICE_COUNT        49
#define DEV_PER_INT         8

/************************************/

/* Status register constants */
#define ALLOFF  0x00000000
#define UMON    0x00000008      /* User Mode ON */
#define UMOFF   0x00000008      /* User Mode OFF (kernel on) */
#define IMON    0x0000FF00      /* Interrupt Masked */
#define IEPON	0x00000004      /* Turn interrupts ON*/
#define IECON	0x00000001      /* Turn interrupts current ON */
#define TEBITON	0x08000000		/* Timer enabled bit ON */
#define CAUSE	0x0000007C		/* Turn on the cause bits for exception handling */
#define CLEARCAUSE 0xFFFFFF00

/* Memory Management Registers */
#define ASIDMASK       0x00000FC0    /* ASID field mask */
#define VMPGMASK       0x00000FFF    /* VPN field mask */
#define PTEMAGICNO     0x2A          /* Page Table Entry magic number */
#define VPNMASK        0xFFFFF000    /* Mask for VPN */
#define ENTRYHIMASK    0xFFFFF000    /* Mask for EntryHI */

/* Exception State Areas */
#define SYSCALL_NEWAREA   0x20001000    /* Syscall new area */
#define SYSCALL_OLDAREA   0x20001100    /* Syscall old area */
#define PGTRAP_NEWAREA    0x20001200    /* PgmTrap new area */
#define PGTRAP_OLDAREA    0x20001300    /* PgmTrap old area */
#define TLB_NEWAREA       0x20001400    /* TLB new area */
#define TLB_OLDAREA       0x20001500    /* TLB old area */
#define INT_NEWAREA       0x20001600    /* Interrupt new area */
#define INT_OLDAREA       0x20001700    /* Interrupt old area */

/* Stack frame sizes and locations */
#define FRAMESIZE      0x00001000    /* Size of a frame - 4KB */
#define RAMTOP         0x20001000    /* Top of RAM for bootstrap */

/* Error Codes */
#define CREATENOGOOD   -1            /* Create process failure */
#define TERMINATENOGOOD -1           /* Terminate process failure */
#define SPECTRAPHCLOSE  -1           /* Close tape device failure */
#define SPECTRAPHINIT   -1           /* Initialize tape device failure */

/* Exception Types */
#define INTERRUPTS         0
#define TLBMOD             1
#define TLBINVLD           2
#define TLBINVLDL          3
#define ADDRINVLD          4
#define ADDRINVLDS         5
#define BUSINVLD           6
#define BUSINVLDL          7
#define SYSCALL_EXCEPTION  8
#define BREAKPOINT         9
#define RESERVEDINST       10
#define COPROCUNUSABLE     11
#define ARITHOVERFLOW      12

/* Exceptions constants */
#define	PGFAULTEXCEPT	    0
#define GENERALEXCEPT	    1

/* Miscellaneous */
#define UPROCMAX       8            /* Maximum number of user processes */
#define PROCESS_PRIO   1            /* Default process priority */
#define SERTEINT       2            /* Serial transmit interrupt */
#define SERTRINT       5            /* Serial receive interrupt */

#endif








