#ifndef _PROC_H_
#define _PROC_H_

#include "stackframe.h"

#include <stdint.h>

#define PROC_MAX 256
#define PROC_NAME_MAX 16

struct proc {
    struct stackframe regs; /* must be at the beginning of proc struct */
    struct segframe segs;

    int counter;          /* remaining ticks */
    int quantum;          /* time slice */
    uint64_t last_cycles; /* cycles at the last context switch */

    /* process state */
#define PST_NONE 0
#define PST_FREESLOT 0x100 /* proc table entry is free */
    int state;

    char name[PROC_NAME_MAX];
};

#endif
