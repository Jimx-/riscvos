#include "global.h"
#include "proto.h"

/* interrupt causes */
#define INTERRUPT_CAUSE_TIMER 5

#define INTERRUPT_CAUSE_FLAG (1UL << (__riscv_xlen - 1))

void handle_irq(reg_t scause)
{
    printk("Interrupt\r\n");

    switch (scause & ~INTERRUPT_CAUSE_FLAG) {
    case INTERRUPT_CAUSE_TIMER:
        timer_interrupt();
        break;
    default:
        break;
    }
}
