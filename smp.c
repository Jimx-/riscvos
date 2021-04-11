#include "smp.h"
#include "const.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"
#include "string.h"

unsigned int hart_counter = 0;

#define BOOT_CPULOCALS_OFFSET 0
unsigned long __cpulocals_offset[CONFIG_SMP_MAX_CPUS] = {
    [0 ... CONFIG_SMP_MAX_CPUS - 1] = BOOT_CPULOCALS_OFFSET,
};

void* __cpu_stack_pointer[CONFIG_SMP_MAX_CPUS];
void* __cpu_task_pointer[CONFIG_SMP_MAX_CPUS];

static volatile int smp_commenced = 0;
static unsigned int cpu_nr;
static unsigned int bsp_hart_id;
static volatile unsigned int __cpu_ready;

extern char k_stacks_start;
static void* k_stacks;
#define get_k_stack_top(cpu) \
    ((void*)(((char*)(k_stacks)) + 2 * ((cpu) + 1) * KSTACK_SIZE))

DEFINE_CPULOCAL(unsigned int, cpu_number);

static void smp_start_cpu(int hart_id)
{
    unsigned int cpuid = cpu_nr++;

    __cpu_ready = -1;
    cpu_to_hart_id[cpuid] = hart_id;
    hart_to_cpu_id[hart_id] = cpuid;

    __asm__ __volatile__("fence rw, rw" : : : "memory");

    __cpu_stack_pointer[hart_id] = get_k_stack_top(cpuid);
    __cpu_task_pointer[hart_id] = (void*)(uintptr_t)cpuid;

    while (__cpu_ready != cpuid)
        ;
}

static int fdt_scan_hart(void* blob, unsigned long offset, const char* name,
                         int depth, void* arg)
{
    const char* type = fdt_getprop(blob, offset, "device_type", NULL);
    if (!type || strcmp(type, "cpu") != 0) return 0;

    const uint32_t* reg = fdt_getprop(blob, offset, "reg", NULL);
    if (!reg) return 0;

    uint32_t hart_id = be32_to_cpup(reg);
    if (hart_id >= CONFIG_SMP_MAX_CPUS) return 0;

    if (hart_id == bsp_hart_id) return 0;

    smp_start_cpu(hart_id);

    return 0;
}

static void setup_cpulocals(void)
{
    size_t size;
    char* ptr;
    int cpu;
    extern char _cpulocals_start[], _cpulocals_end[];

    size = roundup(_cpulocals_end - _cpulocals_start, PG_SIZE);
    ptr = vmalloc_pages((size * cpu_nr) >> PG_SHIFT, NULL);

    for (cpu = 0; cpu < cpu_nr; cpu++) {
        cpulocals_offset(cpu) = ptr - (char*)_cpulocals_start;
        memcpy(ptr, (void*)_cpulocals_start, _cpulocals_end - _cpulocals_start);

        get_cpu_var(cpu, cpu_number) = cpu;

        ptr += size;
    }
}

void init_smp(unsigned int bsp_hart, void* dtb)
{
    bsp_hart_id = bsp_hart;

    cpu_to_hart_id[0] = bsp_hart_id;
    hart_to_cpu_id[bsp_hart_id] = 0;

    cpu_nr++;

    k_stacks = &k_stacks_start;

    of_scan_fdt(fdt_scan_hart, NULL, dtb);

    setup_cpulocals();
}

void smp_boot_ap(void)
{
    __cpu_ready = smp_processor_id();
    printk("smp: CPU %d is up\r\n", smp_processor_id());

    while (!smp_commenced)
        ;

    while (1)
        ;
}

void smp_commence(void)
{
    __asm__ __volatile__("fence rw, rw" : : : "memory");
    smp_commenced = 1;
}
