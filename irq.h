#ifndef _IRQ_H_
#define _IRQ_H_

#include "csr.h"

static inline void local_irq_enable(void) { csr_set(sstatus, SR_SIE); }

static inline void local_irq_disable(void) { csr_clear(sstatus, SR_SIE); }

#endif
