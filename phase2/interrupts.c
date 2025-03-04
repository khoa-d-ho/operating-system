/****************************************************************************
 * interrupts.c
 * 
 * This file implements the device/timer interrupt exception
 * handler. This module will process all the device/timer interrupts, converting
 * device/timer interrupts into V operations on the appropriate semaphores.
 *
 * Written by Khoa Ho & Hieu Tran
 * February 2025
 ****************************************************************************/

#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/scheduler.h"
#include "../h/interrupts.h"
#include "../h/initial.h"
#include "../h/exceptions.h"

/* Function to identify highest priority pending interrupt */
HIDDEN int getHighestPriority(unsigned int cause) {
    int line;
    /* Check interrupt lines, starting with highest priority (1) */
    for (line = 1; line <= TERMINT; line++) {
        if ((cause & (1 << (line + DEV_PER_INT))) != 0)
            return line;
    }
    return -1; /* No pending interrupt */
}

/* Function to find device with pending interrupt on a line */
HIDDEN int getDevice(int line) {
    if (line < DISKINT || line > TERMINT)
        return -1;
    
    /* Get the interrupting devices bitmap for this line */
    memaddr* intDevBitMap = (memaddr*) (INTDEVSBITMAP + (line - DISKINT) * WORDLEN);
    unsigned int bitmap = *intDevBitMap;
    
    int dev;
    /* Find the lowest device number with an interrupt pending */
    for (dev = 0; dev < DEV_PER_INT; dev++) {
        if ((bitmap & (1 << dev)) != 0)
            return dev;
    }
    return -1; /* No device found */
}

/* Main interrupt handler */
void interruptHandler() {
    /* Get cause register to identify pending interrupts */
    unsigned int cause = getCAUSE();
    
    /* Get highest priority interrupt line with pending interrupt */
    int line = getHighestPriority(cause);
    
    if (line == -1) {
        /* No interrupt found - shouldn't happen */
        PANIC();
    } else if (line == PLT_LINE) {
        /* Processor Local Timer interrupt */
        pltInterruptHandler();
    } else if (line == IT_LINE) {
        /* Interval Timer interrupt */
        intervalTimerHandler();
    } else if (line >= 3 && line <= 7) {
        /* Device interrupt */
        deviceInterruptHandler(line);
    }

    /* After handling the interrupt, call scheduler to select next process */
    scheduler();
}

/* Handler for Processor Local Timer interrupts */
void pltInterruptHandler() {
    /* Get old processor state from BIOS data page */
    state_t *oldState = (state_t *) BIOSDATAPAGE;
    
    /* Acknowledge the PLT interrupt by loading a new value */
    LDIT(QUANTUM);
    
    if (currentProcess != NULL) {
        /* Increase CPU time used by current process */
        currentProcess->p_time += QUANTUM;
        
        /* Save processor state to PCB */
        copyState(&(currentProcess->p_s), oldState);
        
        /* Put current process back to ready queue */
        insertProcQ(&readyQueue, currentProcess);
        
        /* Clear current process pointer */
        currentProcess = NULL;
    }
}

/* Handler for Interval Timer interrupts */
void intervalTimerHandler() {
    /* Acknowledge the interval timer interrupt */
    LDIT(CLOCKINTERVAL);
    
    /* Get pointer to the clock semaphore */
    int *clockSem = &(deviceSemaphores[CLOCKINTERVAL]);
    
    /* Unblock all processes waiting on the clock semaphore */
    pcb_PTR p;
    while ((p = removeBlocked(clockSem)) != NULL) {
        insertProcQ(&readyQueue, p);
        softBlockCount--;
    }
}

/* Handler for device interrupts */
void deviceInterruptHandler(int line) {
    int dev = getDevice(line);
    
    if (dev == -1) {
        /* No device found with pending interrupt on this line */
        return;
    }
    
    /* Terminal devices have two subdevices (transmit and receive) */
    if (line == 7) {
        /* Check if it's a transmit or receive interrupt */
        memaddr transStatus = DEV_REG_FIELD(line, dev, 2); /* Transmit status */
        memaddr recvStatus = DEV_REG_FIELD(line, dev, 0);  /* Receive status */
        
        /* Handle transmit interrupts first (higher priority) */
        if ((transStatus & 0xFF) != 0) {
            handleTerminal(line, dev, 1, transStatus);
        }
        /* Then handle receive interrupts */
        else if ((recvStatus & 0xFF) != 0) {
            handleTerminal(line, dev, 0, recvStatus);
        }
    } else {
        /* For non-terminal devices */
        memaddr status = DEV_REG_FIELD(line, dev, 0);
        handleDevice(line, dev, status);
    }
}

/* Handle terminal device interrupts */
void handleTerminal(int line, int dev, int isTransmit, unsigned int status) {
    /* Calculate semaphore index */
    int semIndex;
    if (isTransmit) {
        semIndex = DEV_PER_INT * (line - DISKINT) + dev + DEV_PER_INT; /* Transmit */
    } else {
        semIndex = DEV_PER_INT * (line - DISKINT) + dev;               /* Receive */
    }
    
    /* Get the semaphore */
    int *sem = &(deviceSemaphores[semIndex]);
    
    /* Acknowledge the interrupt */
    if (isTransmit) {
        DEV_REG_FIELD(line, dev, 3) = ACK; /* Transmit command */
    } else {
        DEV_REG_FIELD(line, dev, 1) = ACK; /* Receive command */
    }
    
    /* Perform V operation - increment semaphore */
    (*sem)++;
    
    /* Unblock a process waiting on this semaphore */
    if (*sem <= 0) {
        pcb_PTR p = removeBlocked(sem);
        if (p != NULL) {
            /* Store status in v0 register */
            p->p_s.s_v0 = status;
            
            /* Add to ready queue */
            insertProcQ(&readyQueue, p);
            
            /* Decrease blocked count */
            softBlockCount--;
        }
    }
}

/* Handle non-terminal device interrupts */
void handleDevice(int line, int dev, unsigned int status) {
    /* Calculate semaphore index */
    int semIndex = DEV_PER_INT * (line - 3) + dev;
    
    /* Get the semaphore */
    int *sem = &(deviceSemaphores[semIndex]);
    
    /* Acknowledge the interrupt */
    DEV_REG_FIELD(line, dev, 1) = ACK;
    
    /* Perform V operation - increment semaphore */
    (*sem)++;
    
    /* Unblock a process waiting on this semaphore */
    if (*sem <= 0) {
        pcb_PTR p = removeBlocked(sem);
        if (p != NULL) {
            /* Store status in v0 register */
            p->p_s.s_v0 = status;
            
            /* Add to ready queue */
            insertProcQ(&readyQueue, p);
            
            /* Decrease blocked count */
            softBlockCount--;
        }
    }
}