# The PandOS Operating System 

This repository contains a 5-phase implementation of a lightweight operating system built on the µMPS3 emulator. The project explores foundational OS concepts such as process management, exception handling, virtual memory, and device I/O using semaphores and DMA.



## 1. Project Structure

```
OPERATING-SYSTEM/
├── h/                     - Common header files (declarations, constants, types)
│   └── *.h
├── pandos-src/            - Core kernel implementation files (phases 1–5)
│   ├── asl.c              - Active Semaphore List (ASL) management
│   ├── pcb.c              - Process Control Block (PCB) allocation and tree/queue operations
│   ├── scheduler.c        - Preemptive round-robin scheduler
│   ├── exceptions.c       - Kernel-level exception and SYSCALL (1–8) handlers
│   ├── interrupts.c       - Device interrupt handling and pseudo-clock logic
│   ├── initial.c          - OS bootstrap and Pass-Up Vector initialization
│   ├── initProc.c         - User process and delay daemon initialization
│   ├── vmSupport.c        - Pager implementation and swap pool logic (TLB refill, page fault)
│   ├── sysSupport.c       - Support-level exception handlers and SYSCALLs 9–18
│   ├── delayDaemon.c      - Delay daemon process and Active Delay List (ADL) management
│   ├── deviceSupportDMA.c - DMA-based disk/flash I/O routines (DISKPUT, FLASHPUT, etc.)
│   └── Makefile           - Build configuration for compiling the kernel
├── testers/               - User-level test programs compiled as .umps images
│   ├── h/                 - Headers for test utilities
│   └── *.c                - Test programs (e.g., fib, I/O, delay, VM)
└── README.md              - This documentation file
```

## 2. Development Timeline

All code lives in `pandos-src/`, but development followed the PandOS phase structure. 

### Phase 1: PCB and ASL

Implemented the core kernel data structures:
- `pcb.c`: Process Control Blocks (queues and parent-child trees)
- `asl.c`: Active Semaphore List for process synchronization

### Phase 2: Scheduler and Basic SYSCALLs

Built the core kernel loop and interrupt system:
- `scheduler.c`: Round-robin scheduler with preemption
- `exceptions.c`: SYSCALLs 1–8 (kernel-level system services)
- `interrupts.c`, `initial.c`: Timer/device interrupts and OS bootstrap

### Phase 3: Virtual Memory and User-Level Support

Added paging and user program management:
- `vmSupport.c`: Pager and swap pool using FIFO replacement
- `sysSupport.c`: SYSCALLs 9–13 (e.g., I/O, termination)
- `initProc.c`: Initializes user processes and support structs

### Phase 4: DMA Device I/O

Completed block device operations after delay logic:
- `deviceSupportDMA.c`: Handles disk and flash I/O via DMA
- `sysSupport.c`: SYSCALLs 14–17 (DISKPUT, FLASHPUT, etc.)

### Phase 5: The Delay Facility 

Implemented timed suspension via the Active Delay List (ADL) and a delay daemon:
- `delayDaemon.c`: Handles process delays and wakeups
- `sysSupport.c`: SYSCALL 18 (DELAY) and ADL integration


## 3. Test Suite

User-mode test programs simulate a range of OS features:

- **Terminal I/O:** `terminalTest1.c` – `terminalTest8.c`
- **Printer & Time:** `printerTest.c`, `timeOfDay.c`
- **DMA Tests:** `diskIOtest.c`, `flashIOtest.c`
- **Delay:** `delayTest.c`
- **Computation/Recursion:** `fibSeven.c` – `fibEleven.c`, `pascal11Max.c`
- **String Operations:** `strConcat.c`, `reverseString.c`
- **VM Stress:** `swapStress.c`
- **Visuals & Art:** `asciiArt.c`

Testers are compiled into `.umps` images and run alongside the kernel in the µMPS3 emulator.


## 4. Build and Run Instructions

### Prerequisites

- A Linux environment with µMPS3 emulator [installed](https://wiki.virtualsquare.org/education/tutorials/umps/installation.html) 
- MIPS cross-compiler toolchain (e.g., `mipsel-linux-gnu-gcc`, `mipsel-linux-gnu-ld`)

### Build Kernel

```bash
cd pandos-src/
make
```

### Build Test Programs

```bash
cd testers/
make
```

### Run in µMPS3

To run a test like `fibSeven.umps` with the current kernel:

```bash
umps3 -k pandos-src/kernel.core.umps -f testers/fibSeven.umps
```

For tests requiring a disk:

```bash
umps3 -k pandos-src/kernel.core.umps -f testers/diskIOtest.umps -d0 testers/disk0.umps
```

Use µMPS3’s interface to examine output via TERMINAL0 or associated memory buffers.


## 5. Authors

- **Hieu Tran** – tran_h10@denison.edu
- **Khoa Ho** – ho_d1@dension.edu

This project was developed for Denison University's **CS 372 – Operating Systems** course, instructed by Dr. Mikey Goldweber. 


If this repository helped you in any way, feel free to shoot us an email 😉
