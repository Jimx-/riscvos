#ifndef _VM_H_
#define _VM_H_

#include "csr.h"

/* If we use all 64 bits in the address, there will be 2^64 bytes directly
 * addressable, which is far beyond possible physical RAM size but */
#define KERNEL_VMA \
    0xffffffe000000000UL /* virtual address where the kernel is loaded */
#define IOMAP_BASE 0xffffffff00000000UL

#define USER_STACK_TOP 0x2000000000
#define USER_STACK_SIZE 0x1000

#ifndef __ASSEMBLY__

#include "list.h"
#include "stackframe.h"
#include <stddef.h>

typedef unsigned long pde_t;  /* page directory entry */
typedef unsigned long pmde_t; /* page middle directory entry */
typedef unsigned long pte_t;  /* page table entry */

struct vm_context {
    reg_t ptbr_phys;
    reg_t* ptbr_vir;

    struct list_head regions;
};

struct vm_region {
    struct list_head list;

    void* vir_base;
    unsigned long phys_base;
    size_t size;
};

#endif

#define PGD_SHIFT (30) /* page directory number shift */

#define PMD_SHIFT (21)
#define PMD_SIZE (1UL << PMD_SHIFT)
#define PMD_MASK (~(PMD_SIZE - 1))

#define PG_SIZE 0x1000 /* page size */
#define PG_SHIFT (12)  /* page base shift */

#define PG_PFN_SHIFT 10

/* number of entries in page directory, page middle directory and page table */
#define NUM_DIR_ENTRIES (PG_SIZE / sizeof(pde_t))
#define NUM_PMD_ENTRIES (PG_SIZE / sizeof(pmde_t))
#define NUM_PT_ENTRIES (PG_SIZE / sizeof(pte_t))

/* page table bits */
#define _PG_PRESENT (1 << 0)
#define _PG_READ (1 << 1)
#define _PG_WRITE (1 << 2)
#define _PG_EXEC (1 << 3)
#define _PG_USER (1 << 4)
#define _PG_GLOBAL (1 << 5)
#define _PG_ACCESSED (1 << 6)
#define _PG_DIRTY (1 << 7)

#define _PG_TABLE _PG_PRESENT

/* page permissions */
#define _PROT_BASE (_PG_PRESENT | _PG_ACCESSED | _PG_USER | _PG_DIRTY)

#define PROT_NONE 0
#define PROT_READ (_PROT_BASE | _PG_READ)
#define PROT_WRITE (_PROT_BASE | _PG_READ | _PG_WRITE)
#define PROT_EXEC (_PROT_BASE | _PG_EXEC)
#define PROT_EXEC_READ (_PROT_BASE | _PG_READ | _PG_EXEC)
#define PROT_EXEC_WRITE (_PROT_BASE | _PG_READ | _PG_WRITE | _PG_EXEC)

#define PROT_KERNEL \
    (_PG_PRESENT | _PG_READ | _PG_WRITE | _PG_ACCESSED | _PG_DIRTY)
#define PROT_KERNEL_EXEC (PROT_KERNEL | _PG_EXEC)

#ifndef __ASSEMBLY__
extern pde_t initial_pgd[];

#define PTE_INDEX(v) (((unsigned long)(v) >> PG_SHIFT) & (NUM_PT_ENTRIES - 1))
#define PMDE_INDEX(x) \
    (((unsigned long)(x) >> PMD_SHIFT) & (NUM_PMD_ENTRIES - 1))
#define PDE_INDEX(x) (((unsigned long)(x) >> PGD_SHIFT) & (NUM_DIR_ENTRIES - 1))

/* helper functions */
static inline pde_t pfn_pde(unsigned long pfn, unsigned long prot)
{
    return (pfn << PG_PFN_SHIFT) | prot;
}

static inline pmde_t pfn_pmde(unsigned long pfn, unsigned long prot)
{
    return (pfn << PG_PFN_SHIFT) | prot;
}

static inline pte_t pfn_pte(unsigned long pfn, unsigned long prot)
{
    return (pfn << PG_PFN_SHIFT) | prot;
}

#define __pa(x) ((unsigned long)(x)-va_pa_offset)
#define __va(x) ((void*)((unsigned long)(x) + va_pa_offset))

static inline void enable_user_access()
{
    __asm__ __volatile__("csrs sstatus, %0" : : "r"(SR_SUM) : "memory");
}

static inline void disable_user_access()
{
    __asm__ __volatile__("csrc sstatus, %0" : : "r"(SR_SUM) : "memory");
}

static inline void mb() { __asm__ __volatile__("sfence.vma" : : : "memory"); }

static inline void flush_tlb()
{
    __asm__ __volatile__("sfence.vma" : : : "memory");
}

static inline unsigned long read_ptbr()
{
    unsigned long ptbr = csr_read(sptbr);
    return (unsigned long)((ptbr & SATP_PPN) << PG_SHIFT);
}

static inline void write_ptbr(unsigned long ptbr)
{
    flush_tlb();
    csr_write(sptbr, (ptbr >> PG_SHIFT) | SATP_MODE);
}

#endif

#endif
