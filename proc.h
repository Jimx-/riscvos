#ifndef _PROC_H_
#define _PROC_H_

#include "list.h"
#include "stackframe.h"
#include "vm.h"

#include <stdint.h>

#define PROC_MAX 256
#define PROC_NAME_MAX 16

struct proc {
    struct reg_context regs; /* must be at the beginning of proc struct */
    struct thread_info thread;
    struct vm_context vm;
    uint8_t* kernel_stack;

    unsigned long counter_ns; /* remaining ticks */
    int quantum_ms;           /* time slice */

    /* process state */
#define PST_NONE 0
#define PST_FREESLOT 0x100 /* proc table entry is free */
    int state;

    char name[PROC_NAME_MAX];
};

#endif
