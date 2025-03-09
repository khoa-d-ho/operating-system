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
#define INTDEVSBITMAP  0x10000040  /* Interrupting Devices Bit Map */

/* Exception code macros */
#define CAUSE_EXCCODE_MASK 0x0000007C
#define CAUSE_EXCCODE_SHIFT 2
#define CAUSE_GET_EXCCODE(cause) ((cause & CAUSE_EXCCODE_MASK) >> CAUSE_EXCCODE_SHIFT)

/* utility constants */
#define	TRUE			    1
#define	FALSE			    0
#define HIDDEN			  static
#define EOS				    '\0'
#define MAXINT       0x7FFFFFFF
#define KUPON         0x00000008
#define KUPOFF        0xFFFFFFF7
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
#define DEVREGSIZE	      16 	   /* device register size in bytes */

/* Device Interrupt Masks */
#define DEV0 	0x00000001
#define DEV1 	0x00000002
#define DEV2 	0x00000004
#define DEV3 	0x00000008
#define DEV4 	0x00000010
#define DEV5 	0x00000020
#define DEV6 	0x00000040
#define DEV7 	0x00000080

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
#define CLOCKINTERVAL   100000UL    /* 100ms in microseconds */

/* system call codes */
#define CREATEPROCESS    1
#define TERMPROCESS      2
#define PASSEREN         3
#define VERHOGEN         4
#define WAITFORIO        5
#define GETCPUTIME       6
#define WAITFORCLOCK     7
#define GETSUPPORT       8

/*Line Constants*/
#define PROCESSOR 0
#define INTERVALTIMER 1
#define PLTIMER 2
#define DISK 3
#define FLASH 4
#define NETWORK 5
#define PRINTER 6
#define TERMINAL 7

/* Device Constants */
#define DEVICE_COUNT        49
#define DEV_PER_INT         8
#define CLOCK           DEVICE_COUNT - 1

/************************************/

/* Status register constants */
#define ALLOFF  0x0
#define UMON    0x00000008      /* User Mode ON */
#define UMOFF   0x00000008      /* User Mode OFF (kernel on) */
#define IMON    0x0000FF00      /* Interrupt Masked */
#define IEPON	0x00000004      /* Turn interrupts ON*/
#define IECON	0x00000001      /* Turn interrupts current ON */
#define TEBITON	0x08000000		/* Timer enabled bit ON */
#define CAUSE	0x0000007C		/* Turn on the cause bits for exception handling */
#define CLEARCAUSE 0xFFFFFF00
#define XLVALUE 0xFFFFFFFF

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

/* Interrupts in Cause Reg */
#define INTERRUPT_PLT      0x00000200
#define INTERRUPT_IT       0x00000400
#define INTERRUPT_DISK     0x00000800
#define INTERRUPT_FLASH    0x00001000
#define INTERRUPT_NETW     0x00002000
#define INTERRUPT_PRINT    0x00004000
#define INTERRUPT_TERM    0x00008000

/* Stack frame sizes and locations */
#define FRAMESIZE      0x00001000    /* Size of a frame - 4KB */
#define NUCLEUSSTACKTOP         0x20001000    /* Top of RAM for bootstrap */

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

/* Constants for interrupt handling */
#define PLT_LINE 1
#define IT_LINE 2

/* Structure for accessing device registers */
#define DEV_REG_ADDRESS(line, dev) (memaddr) (0x10000054 + ((line - 3) * 0x80) + (dev * 0x10))
#define DEV_REG_FIELD(line, dev, field) (* ((memaddr *) (DEV_REG_ADDRESS(line, dev) + field * 0x4)))

/* Miscellaneous */
#define UPROCMAX       8            /* Maximum number of user processes */
#define PROCESS_PRIO   1            /* Default process priority */
#define SERTEINT       2            /* Serial transmit interrupt */
#define SERTRINT       5            /* Serial receive interrupt */

#endif
