#include "proc.h"
#include "global.h"
#include "proto.h"

void init_proc()
{
    int i;
    for (i = 0; i < PROC_MAX; i++) {
        struct proc* p = &proc_table[i];

        p->state = PST_FREESLOT;
    }
}

void switch_to_user()
{
    struct proc* p;
    extern char KStackTop;

    p = pick_proc();

    /* set kernel stack */
    p->regs.kernel_sp = (reg_t)&KStackTop;
    switch_address_space(p);

    restore_user_context(p);
}
