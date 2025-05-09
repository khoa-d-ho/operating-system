#include "h/localLibumps.h"
#include "h/tconst.h"
#include "h/print.h"

/**********************************************************************
 * flashIOtest.c
 *
 * This file contains a test for the flash device I/O operations.
 * It writes to two different blocks and reads back the values. It also
 * tests the flash read operation into protected RAM.
 *
 * Written by Khoa Ho & Hieu Tran
 */

#define MILLION 1000000
#define VALID_BLOCK_START 32  

void main() {
    int i;
    int fstatus;
    int *buffer;

    buffer = (int *)(SEG2 + (21 * PAGESIZE)); 

    print(WRITETERMINAL, "flashTest starts\n");

    /* test writing and reading back from two different blocks */
    *buffer = 2025;
    fstatus = SYSCALL(FLASH_PUT, (int)buffer, 0, VALID_BLOCK_START);
    if (fstatus != READY)
        print(WRITETERMINAL, "flashTest error: flash write 1 failed\n");

    *buffer = 9999;
    fstatus = SYSCALL(FLASH_PUT, (int)buffer, 0, VALID_BLOCK_START + 1);
    if (fstatus != READY)
        print(WRITETERMINAL, "flashTest error: flash write 2 failed\n");

    /* test readback from first block */
    *buffer = 0;
    fstatus = SYSCALL(FLASH_GET, (int)buffer, 0, VALID_BLOCK_START);
    if (*buffer != 2025)
        print(WRITETERMINAL, "flashTest error: bad flash block 1 readback\n");
    else
        print(WRITETERMINAL, "flashTest ok: flash block 1 readback\n");

    /* test readback from second block */
    *buffer = 0;
    fstatus = SYSCALL(FLASH_GET, (int)buffer, 0, VALID_BLOCK_START + 1);
    if (*buffer != 9999)
        print(WRITETERMINAL, "flashTest error: bad flash block 2 readback\n");
    else
        print(WRITETERMINAL, "flashTest ok: flash block 2 readback\n");

    /* try to do a flash read into protected RAM */
    /* SYSCALL(FLASH_GET, SEG1, 0, VALID_BLOCK_START); */

    print(WRITETERMINAL, "flashTest: completed\n");

    SYSCALL(TERMINATE, 0, 0, 0);
}