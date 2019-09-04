#include "global.h"
#include "proc.h"

#include <stddef.h>

/* choose ONE process to run */
struct proc* pick_proc()
{
    unsigned long greatest_counter = 0;
    struct proc *p, *p_next = NULL;

    while (!greatest_counter) {
        for (p = proc_table; p < proc_table + PROC_MAX; p++) {
            if (p->state & PST_FREESLOT) {
                continue;
            }

            if (p->counter_ns > greatest_counter) {
                greatest_counter = p->counter_ns;
                p_next = p;
            }
        }

        if (!greatest_counter) {
            /* refill the counters */
            for (p = proc_table; p < proc_table + PROC_MAX; p++) {
                if (p->state & PST_FREESLOT) {
                    continue;
                }

                p->counter_ns = p->quantum_ms * 1000000ULL;
            }
        }
    }

    return p_next;
}
