#include "vm.h"
#include "const.h"
#include "global.h"
#include "proc.h"
#include "proto.h"

#include <stdint.h> /* for uintptr_t */
#include <string.h>

pde_t initial_pgd[NUM_DIR_ENTRIES] __attribute__((aligned(PG_SIZE)));

/* Initial page mapping
 * va[0xffffffe000000000-0xffffffffffffffff] ->
 * pa[0x0000000000000000-0x0000002000000000]
 */
#define NUM_INIT_PMDS ((unsigned long)-KERNEL_VMA >> PGD_SHIFT)
pmde_t initial_pmd[NUM_PMD_ENTRIES * NUM_INIT_PMDS]
    __attribute__((aligned(PG_SIZE)));

void setup_paging()
{
    /* setup the initial page directory used during boot */
    extern char _start;
    uintptr_t pa_start = (uintptr_t)&_start;
    unsigned long prot = PROT_KERNEL_EXEC;

    va_pa_offset = KERNEL_VMA - pa_start;

    int i;
    for (i = 0; i < NUM_INIT_PMDS; i++) {
        initial_pgd[(KERNEL_VMA >> PGD_SHIFT) % NUM_DIR_ENTRIES + i] =
            pfn_pde(((uintptr_t)initial_pmd >> PG_SHIFT) + i, _PG_TABLE);
    }

    for (i = 0; i < sizeof(initial_pmd) / sizeof(pmde_t); i++) {
        initial_pmd[i] = pfn_pmde((pa_start + i * PMD_SIZE) >> PG_SHIFT, prot);
    }
}

static pte_t* pg_alloc_pt()
{
    unsigned long phys_addr;
    pte_t* pt = (pte_t*)alloc_page(&phys_addr);

    memset(pt, 0, sizeof(pte_t) * NUM_PT_ENTRIES);
    return pt;
}

static pmde_t* pg_alloc_pmd()
{
    unsigned long phys_addr;
    pmde_t* pmd = (pmde_t*)alloc_page(&phys_addr);

    memset(pmd, 0, sizeof(pmde_t) * NUM_PMD_ENTRIES);
    return pmd;
}

static inline pde_t* pgd_offset(pde_t* pgd, unsigned long addr)
{
    return pgd + PDE_INDEX(addr);
}

static inline int pde_present(pde_t pde) { return pde & _PG_PRESENT; }

static inline void pde_populate(pde_t* pde, pmde_t* pmd)
{
    unsigned long pfn = (unsigned long)__pa(pmd) >> PG_SHIFT;
    *pde = (pfn << PG_PFN_SHIFT) | _PG_TABLE;
}

static inline pmde_t* pmd_offset(pde_t* pmd, unsigned long addr)
{
    pmde_t* vaddr = (pmde_t*)__va((*pmd >> PG_PFN_SHIFT) << PG_SHIFT);
    return vaddr + PMDE_INDEX(addr);
}

static inline int pmde_present(pmde_t pmde) { return pmde & _PG_PRESENT; }

static inline void pmde_populate(pmde_t* pmde, pte_t* pt)
{
    unsigned long pfn = (unsigned long)__pa(pt) >> PG_SHIFT;
    *pmde = (pfn << PG_PFN_SHIFT) | _PG_TABLE;
}

static inline pte_t* pte_offset(pmde_t* pt, unsigned long addr)
{
    pte_t* vaddr = (pte_t*)__va((*pt >> PG_PFN_SHIFT) << PG_SHIFT);
    return vaddr + PTE_INDEX(addr);
}

static inline int pte_present(pte_t pte) { return pte & _PG_PRESENT; }

void vm_map(struct proc* p, unsigned long phys_addr, void* vir_addr,
            void* vir_end)
{
    pde_t* pgd = (pde_t*)p->segs.ptbr_vir;

    if (phys_addr % PG_SIZE) phys_addr = roundup(phys_addr, PG_SIZE);

    while (vir_addr < vir_end) {
        unsigned long ph = phys_addr;
        if (ph == 0) {
            alloc_page(&ph);
        }

        pde_t* pde = pgd_offset(pgd, (unsigned long)vir_addr);
        if (!pde_present(*pde)) {
            pmde_t* new_pmd = pg_alloc_pmd();
            pde_populate(pde, new_pmd);
        }

        pmde_t* pmde = pmd_offset(pde, (unsigned long)vir_addr);
        if (!pmde_present(*pmde)) {
            pte_t* new_pt = pg_alloc_pt();
            pmde_populate(pmde, new_pt);
        }

        pte_t* pte = pte_offset(pmde, (unsigned long)vir_addr);
        *pte = pfn_pte(ph >> PG_SHIFT, PROT_EXEC_WRITE);

        vir_addr += PG_SIZE;
        if (phys_addr != 0) phys_addr += PG_SIZE;
    }
}

void switch_address_space(struct proc* p) { write_ptbr(p->segs.ptbr_phys); }
