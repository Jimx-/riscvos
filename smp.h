#ifndef _SMP_H_
#define _SMP_H_

#include "config.h"
#include "stackframe.h"

#ifndef __ASSEMBLY__

register reg_t __raw_smp_processor_id __asm__("tp");

static inline unsigned int smp_processor_id(void)
{
    return (unsigned int)__raw_smp_processor_id;
}

#ifndef __cpulocals_offset

extern unsigned long __cpulocals_offset[CONFIG_SMP_MAX_CPUS];

#define cpulocals_offset(cpu) __cpulocals_offset[cpu]

#endif /* __cpulocals_offset */

#define CPULOCAL_BASE_SECTION ".data.cpulocals"

#define get_cpu_var_ptr(cpu, name)                      \
    ({                                                  \
        unsigned long __ptr;                            \
        __ptr = (unsigned long)(&(name));               \
        (typeof(name)*)(__ptr + cpulocals_offset(cpu)); \
    })

#define get_cpu_var(cpu, name) (*get_cpu_var_ptr(cpu, name))
#define get_cpulocal_var_ptr(name) get_cpu_var_ptr(smp_processor_id(), name)
#define get_cpulocal_var(name) get_cpu_var(smp_processor_id(), name)

#define DECLARE_CPULOCAL(type, name) \
    extern __attribute__((section(CPULOCAL_BASE_SECTION))) __typeof__(type) name

#define DEFINE_CPULOCAL(type, name) \
    __attribute__((section(CPULOCAL_BASE_SECTION))) __typeof__(type) name

#endif /* __ASSEMBLY__ */

#endif
