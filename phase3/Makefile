#$Id: Makefile,v 1.2 2004/05/01 14:53:48 morsiani Exp morsiani $
# Makefile for mips-linux
#
# Edited for uMPS v3 by MikeyG on 2020/04/14

# Simplistic search for the umps3 installation prefix.
# If you have umps3 installed on some weird location, set UMPS3_DIR_PREFIX by hand.
ifneq ($(wildcard /usr/bin/umps3),)
	UMPS3_DIR_PREFIX = /usr
	LIBDIR = $(UMPS3_DIR_PREFIX)/lib/x86_64-linux-gnu/umps3
	
else
	UMPS3_DIR_PREFIX = /usr/local
	LIBDIR = $(UMPS3_DIR_PREFIX)/lib/umps3
endif

INCDIR = $(UMPS3_DIR_PREFIX)/include/umps3/umps
SUPDIR = $(UMPS3_DIR_PREFIX)/share/umps3
#LIBDIR = $(UMPS3_DIR_PREFIX)/lib/umps3

DEFS = ../h/const.h ../h/types.h ../h/pcb.h ../h/asl.h \
	../h/initial.h ../h/interrupts.h ../h/scheduler.h ../h/exceptions.h \
	../h/initProc.h ../h/vmSupport.h ../h/sysSupport.h \
	$(INCDIR)/libumps.h Makefile

OBJS = asl.o pcb.o \
       initial.o interrupts.o scheduler.o exceptions.o \
       initProc.o vmSupport.o sysSupport.o

CFLAGS = -ffreestanding -ansi -Wall -c -mips1 -mabi=32 -mfp32 -mno-gpopt -G 0 -fno-pic -mno-abicalls

LDAOUTFLAGS = -G 0 -nostdlib -T $(SUPDIR)/umpsaout.ldscript
LDCOREFLAGS =  -G 0 -nostdlib -T $(SUPDIR)/umpscore.ldscript


CC = mipsel-linux-gnu-gcc
LD = mipsel-linux-gnu-ld
AS = mipsel-linux-gnu-as -KPIC

EF = umps3-elf2umps
UDEV = umps3-mkdev

#main target
all: kernel.core.umps 

kernel.core.umps: kernel
	$(EF) -k kernel

kernel: $(OBJS)
	$(LD) $(LDCOREFLAGS) $(LIBDIR)/crtso.o $(OBJS) $(LIBDIR)/libumps.o -o kernel

%.o: %.c $(DEFS)
	$(CC) $(CFLAGS) $<


clean:
	rm -f *.o *.umps kernel


distclean: clean
	-rm kernel.*.umps tape0.umps
