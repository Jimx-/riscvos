#include "csr.h"
#include "proc.h"
#include "sbi.h"

#include <stdint.h>

/* TODO: need to calibrate timer!!!!! */

uint64_t read_cycles()
{
    uint64_t n;

    /* TODO: spike complains about this, should use mtime to get cycles */
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
}

void restart_local_timer()
{
    csr_set(sie, SIE_STIE);
    uint64_t cycles = read_cycles();
    sbi_set_timer(cycles + 1000);
}

void timer_interrupt() { csr_clear(sie, SIE_STIE); }

void stop_context(struct proc* p)
{
    uint64_t cycles = read_cycles();
    uint64_t delta = cycles - p->last_cycles;
}
