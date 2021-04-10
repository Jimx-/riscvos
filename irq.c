#include "fdt.h"
#include "global.h"
#include "proto.h"

#include "smp.h"

/* interrupt causes */
#define INTERRUPT_CAUSE_FLAG (1UL << (__riscv_xlen - 1))
#define INTERRUPT_CAUSE_TIMER 5
#define INTERRUPT_CAUSE_EXT 9

#define PRIORITY_BASE 0
#define PRIORITY_PER_ID 4

#define ENABLE_BASE 0x2000
#define ENABLE_PER_HART 0x80

#define CONTEXT_BASE 0x200000
#define CONTEXT_PER_HART 0x1000
#define CONTEXT_THRESHOLD 0x00
#define CONTEXT_CLAIM 0x04

#define PLIC_DISABLE_THRESHOLD 0x7
#define PLIC_ENABLE_THRESHOLD 0

#define NR_IRQS 35
struct irq_handler {
    struct list_head list;
    int (*handler)(int irq, void*);
    void* data;
};
static struct list_head irq_handlers[NR_IRQS];

struct plic {
    void* regs;
};

struct plic_context {
    void* hart_base;
    void* enable_base;

    struct plic* plic;
};

DEFINE_CPULOCAL(struct plic_context, plic_contexts);

static inline void plic_toggle(struct plic_context* ctx, int hwirq, int enable)
{
    uint32_t* reg = ctx->enable_base + (hwirq / 32) * sizeof(uint32_t);
    uint32_t hwirq_mask = 1 << (hwirq % 32);
    uint32_t val = *(volatile uint32_t*)reg;

    if (enable)
        *(volatile uint32_t*)reg = val | hwirq_mask;
    else
        *(volatile uint32_t*)reg = val & ~hwirq_mask;
}

static int fdt_scan_plic(void* blob, unsigned long offset, const char* name,
                         int depth, void* arg)
{
    const char* type = fdt_getprop(blob, offset, "compatible", NULL);
    int cpu = 0;

    if (!type || strcmp(type, "riscv,plic0") != 0) return 0;

    struct plic* plic;
    SLABALLOC(plic);

    const uint32_t* reg;
    int len;
    reg = fdt_getprop(blob, offset, "reg", &len);
    if (!reg) return 0;

    uint64_t base, size;
    base = of_read_number(reg, dt_root_addr_cells);
    reg += dt_root_addr_cells;
    size = of_read_number(reg, dt_root_size_cells);
    reg += dt_root_size_cells;

    plic->regs = vm_mapio(base, size);

    struct plic_context* ctx = get_cpu_var_ptr(cpu, plic_contexts);
    ctx->plic = plic;
    ctx->hart_base = plic->regs + CONTEXT_BASE + 0 * CONTEXT_PER_HART;
    ctx->enable_base = plic->regs + ENABLE_BASE + 0 * ENABLE_PER_HART;

    const uint32_t* ndev;
    ndev = fdt_getprop(blob, offset, "riscv,ndev", &len);
    if (!ndev) return 0;

    int nr_irqs = of_read_number(ndev, 1);

    int hwirq;
    for (hwirq = 1; hwirq <= nr_irqs; hwirq++)
        plic_toggle(ctx, hwirq, 0);

    return 1;
}

static void toggle_irq(struct plic_context* ctx, int hwirq, int enable)
{
    uint32_t* reg = ctx->plic->regs + PRIORITY_BASE + hwirq * PRIORITY_PER_ID;
    *(volatile uint32_t*)reg = enable;

    plic_toggle(ctx, hwirq, enable);
}

void irq_mask(int hwirq)
{
    struct plic_context* ctx = get_cpulocal_var_ptr(plic_contexts);
    toggle_irq(ctx, hwirq, 0);
}

void irq_unmask(int hwirq)
{
    struct plic_context* ctx = get_cpulocal_var_ptr(plic_contexts);
    toggle_irq(ctx, hwirq, 1);
}

void init_irq(void* dtb)
{
    int i;
    for (i = 0; i < NR_IRQS; i++)
        INIT_LIST_HEAD(&irq_handlers[i]);

    of_scan_fdt(fdt_scan_plic, NULL, dtb);
}

static void plic_set_threshold(struct plic_context* ctx, uint32_t threshold)
{
    uint32_t* reg = ctx->hart_base + CONTEXT_THRESHOLD;

    *(volatile uint32_t*)reg = threshold;
}

void init_irq_cpu(int cpu)
{
    struct plic_context* ctx = get_cpu_var_ptr(cpu, plic_contexts);
    csr_set(sie, SIE_SEIE);
    plic_set_threshold(ctx, PLIC_ENABLE_THRESHOLD);
}

static void plic_handle_irq(void)
{
    struct plic_context* ctx = get_cpulocal_var_ptr(plic_contexts);
    uint32_t* claim = ctx->hart_base + CONTEXT_CLAIM;
    int hwirq;

    csr_clear(sie, SIE_SEIE);

    while ((hwirq = *(volatile uint32_t*)claim)) {
        struct irq_handler* handler;

        list_for_each_entry(handler, &irq_handlers[hwirq], list)
        {
            handler->handler(hwirq, handler->data);
        }

        *(volatile uint32_t*)claim = hwirq;
    }

    csr_set(sie, SIE_SEIE);
}

void handle_irq(reg_t scause)
{
    printk("Interrupt\r\n");
    switch (scause & ~INTERRUPT_CAUSE_FLAG) {
    case INTERRUPT_CAUSE_TIMER:
        timer_interrupt();
        break;
    case INTERRUPT_CAUSE_EXT:
        plic_handle_irq();
        break;
    default:
        break;
    }
}

void put_irq_handler(int irq, int (*handler)(int, void*), void* data)
{
    struct irq_handler* irq_handler;
    SLABALLOC(irq_handler);
    if (!irq_handler) return;

    irq_handler->handler = handler;
    irq_handler->data = data;

    list_add(&irq_handler->list, &irq_handlers[irq]);

    irq_unmask(irq);
}
