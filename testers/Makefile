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

TDEFS = h/print.h h/tconst.h $(INCDIR)/libumps.h Makefile

CFLAGS = -ffreestanding -ansi -c -mips1 -mabi=32 -mfp32 -mno-gpopt -G 0 -fno-pic -mno-abicalls
# -Wall

LDAOUTFLAGS = -G 0 -nostdlib -T $(SUPDIR)/umpsaout.ldscript
LDCOREFLAGS =  -G 0 -nostdlib -T $(SUPDIR)/umpscore.ldscript

CC = mipsel-linux-gnu-gcc
LD = mipsel-linux-gnu-ld
AS = mipsel-linux-gnu-as -KPIC

EF = umps3-elf2umps
UDEV = umps3-mkdev

#main target
all: printerTest.umps strConcat.umps terminalReader.umps asciiArt.umps \
	fibSeven.umps fibEight.umps fibNine.umps fibTen.umps fibEleven.umps \
	terminalTest1.umps terminalTest2.umps terminalTest3.umps terminalTest4.umps \
	terminalTest5.umps terminalTest6.umps terminalTest7.umps terminalTest8.umps \
	timeOfDay.umps swapStress.umps pascal11Max.umps reverseString.umps delayTest.umps diskIOtest.umps \
	flashIOtest.umps

	
%.o: %.c $(TDEFS)
	$(CC) $(CFLAGS) $<
	
%.t: %.o print.o  $(LIBDIR)/crti.o
	$(LD) $(LDAOUTFLAGS) $(LIBDIR)/crti.o $< print.o $(LIBDIR)/libumps.o -o $@
	
%.t.aout.umps: %.t
	$(EF) -a $<

%.umps: %.t.aout.umps
	$(UDEV) -f $@ $<


clean:
	rm -f *.o *.t *.umps

