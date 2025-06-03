#include "../h/deviceSupportDMA.h"

/******************************************************************************
 * deviceSupportDMA.c
 * 
 * This file contains the implementation of the device support functions for
 * handling DMA operations on the flash device. It includes functions for
 * performing read and write operations on the flash device.
 * 
 * Written by Khoa Ho & Hieu Tran
 * May 2025
 *****************************************************************************/

/******************************************************************************
 * Function: flashOperation
 * 
 * This function performs a read or write operation on the flash device. It
 * takes the operation type (read/write), device number, block number, and
 * frame address as parameters. It uses the device registers to perform the
 * operation and returns the status of the operation.
 * 
 * Parameters:
 *   operation - FLASH_READBLK (2) or FLASH_WRITEBLK (3)
 *   devNo - flash device number
 *   blockNo - block number
 *   frameAddr - frame address
 * 
 * Returns:
 *   The status of the operation (READY or error code).
 */
int flashOperation(int operation, int devNo, int blockNo, int frameAddr) {
    int devIndex = ((FLASHINT - DISKINT) * DEVPERINT) + devNo;

    /* check if the block number is valid */
    devregarea_t *devRegArea = (devregarea_t *) RAMBASEADDR;
    unsigned int maxBlock = devRegArea->devreg[devIndex].d_data1;
    if (blockNo >= maxBlock) {
        supProgramTrapHandler();  
    }

    /* get mutex for device */
    mutex(ON, &devSemaphore[devIndex]); 

    /* write starting physical address to be R/W to device register */
    devRegArea->devreg[devIndex].d_data0 = frameAddr; 

    toggleInterrupts(OFF);
    /* set command field with device block number & command to read/write  */
    devRegArea->devreg[devIndex].d_command = (blockNo << BITSHIFT_8) | operation;

    /* issue command to device */
    int status = SYSCALL(WAITFORIO, FLASHINT, devNo, 0);
    toggleInterrupts(ON);

    mutex(OFF, &devSemaphore[devIndex]);

    if (status != READY) {
        /* handle error, return negative status code */
        status = -status;
    }
    return status;
}

/*******************************************************************************
 * Function: diskOperation
 * 
 * This function performs a read or write operation on the disk device. It
 * takes the operation type (read/write), device number, sector number, and
 * frame address as parameters. It uses the device registers to perform the
 * operation and returns the status of the operation.
 * 
 * Parameters:
 *   operation - READBLK (3) or WRITEBLK (4)
 *   devNo - disk device number
 *   sectorNo - sector number
 *   frameAddr - frame address
 */
int diskOperation(int operation, int devNo, int sectorNo, int frameAddr) {
    devregarea_t *devRegArea = (devregarea_t *) RAMBASEADDR;

    mutex(ON, &devSemaphore[devNo]);

    /* extract sector geometry from device DATA1 register */
    unsigned int data1 = devRegArea->devreg[devNo].d_data1;
    unsigned int maxSect = data1 & BITMASK_8;
    unsigned int maxHead = (data1 >> BITSHIFT_8) & BITMASK_8;
    unsigned int maxCyl = data1 >> BITSHIFT_16;

    /* check if the sector number is valid */
    if (sectorNo < 0 || sectorNo > (maxCyl * maxHead * maxSect)) {
        supProgramTrapHandler();
    }

    /* translate 1D sector number into (cyl, head, sect) */
    int cyl = sectorNo / (maxHead * maxSect);
    int temp = sectorNo % (maxHead * maxSect);
    int head = temp / maxSect;
    int sect = temp % maxSect;

    /* step 1: seek to cylinder */
    toggleInterrupts(OFF);
    devRegArea->devreg[devNo].d_command = (cyl << BITSHIFT_8) | SEEKCYL;
    int status = SYSCALL(WAITFORIO, DISKINT, devNo, 0); 
    toggleInterrupts(ON);

    if (status == READY) {
        /* write starting physical address (DMA buffer) to device register */
        devRegArea->devreg[devNo].d_data0 = frameAddr;

        /* step 2: set head and sector, if seek was successful */
        toggleInterrupts(OFF);
        devRegArea->devreg[devNo].d_command =
            (head << BITSHIFT_16) | (sect << BITSHIFT_8) | operation;
        status = SYSCALL(WAITFORIO, DISKINT, devNo, 0);
        toggleInterrupts(ON);
    }

    mutex(OFF, &devSemaphore[devNo]);

    if (status != READY) {
        /* handle error, return negative status code */
        status = -status;
    }
    return status;
}


