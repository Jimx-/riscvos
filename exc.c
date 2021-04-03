#include "csr.h"
#include "global.h"
#include "proc.h"
#include "proto.h"

extern void trap_entry(void);

void init_trap()
{
    csr_write(stvec, (reg_t)&trap_entry);
    csr_write(sie, -1);
    csr_clear(sie, SIE_SEIE);
}

void do_trap_unknown(int in_kernel, struct proc* p)
{
    printk("unknown trap\n");
}

void do_trap_insn_misaligned(int in_kernel, struct proc* p)
{
    printk("insn misaligned\n");
}

void do_trap_insn_fault(int in_kernel, struct proc* p)
{
    printk("insn fault\n");
}

void do_trap_insn_illegal(int in_kernel, struct proc* p)
{
    printk("insn illegal\n");
}

void do_trap_break(int in_kernel, struct proc* p) { printk("break\n"); }

void do_trap_load_misaligned(int in_kernel, struct proc* p)
{
    printk("load misaligned\n");
}

void do_trap_load_fault(int in_kernel, struct proc* p)
{
    printk("load fault\n");
}

void do_trap_store_misaligned(int in_kernel, struct proc* p)
{
    printk("store misaligned\n");
}

void do_trap_store_fault(int in_kernel, struct proc* p)
{
    printk("store fault\n");
}

void do_trap_ecall_u(int in_kernel, struct proc* p) { printk("ecall u\n"); }

void do_trap_ecall_s(int in_kernel, struct proc* p) { printk("ecall s\n"); }

void do_trap_ecall_m(int in_kernel, struct proc* p) { printk("ecall m\n"); }

void do_page_fault(int in_kernel, reg_t sbadaddr, reg_t sepc)
{
    printk(
        "page fault[in_kernel: %d, badaddr: %lx, sepc: %lx, ptbr: %lx/%lx]\r\n",
        in_kernel, sbadaddr, sepc, read_ptbr(), __pa(initial_pgd));
}
