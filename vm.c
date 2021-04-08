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
#define NUM_INIT_PMDS ((unsigned long)(IOMAP_BASE - KERNEL_VMA) >> PGD_SHIFT)
pmde_t initial_pmd[NUM_PMD_ENTRIES * NUM_INIT_PMDS]
    __attribute__((aligned(PG_SIZE)));

struct iomap {
    void* vir_base;
    unsigned long phys_addr;
    size_t size;
};

#define IOMAP_MAX 20
static struct iomap iomaps[IOMAP_MAX];
static int num_iomaps = 0;

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
    unsigned long phys_addr = alloc_pages(1);
    pte_t* pt = (pte_t*)__va(phys_addr);

    memset(pt, 0, sizeof(pte_t) * NUM_PT_ENTRIES);
    return pt;
}

static pmde_t* pg_alloc_pmd()
{
    unsigned long phys_addr = alloc_pages(1);
    pmde_t* pmd = (pmde_t*)__va(phys_addr);

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

static void vm_mappage(pde_t* pgd, unsigned long phys_addr, void* vir_addr,
                       void* vir_end, unsigned long prot)
{
    while (vir_addr < vir_end) {
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
        *pte = pfn_pte(phys_addr >> PG_SHIFT, prot);

        vir_addr += PG_SIZE;
        phys_addr += PG_SIZE;
    }
}

void vm_map(struct proc* p, unsigned long phys_addr, void* vir_addr,
            void* vir_end)
{
    size_t mapsize = roundup(vir_end - vir_addr, PG_SIZE);

    if (phys_addr == 0) phys_addr = alloc_pages(mapsize >> PG_SHIFT);
    if (phys_addr % PG_SIZE) phys_addr = roundup(phys_addr, PG_SIZE);

    struct vm_region* vmr;
    SLABALLOC(vmr);
    INIT_LIST_HEAD(&vmr->list);
    vmr->phys_base = phys_addr;
    vmr->vir_base = vir_addr;
    vmr->size = mapsize;
    list_add(&vmr->list, &p->vm.regions);

    vm_mappage((pde_t*)p->vm.ptbr_vir, phys_addr, vir_addr, vir_end,
               PROT_EXEC_WRITE);
}

void vm_mapkernel(struct proc* p)
{
    pde_t* pgd = (pde_t*)p->vm.ptbr_vir;

    extern char _start;
    unsigned long pa_start = __pa(&_start);
    size_t num_pmds =
        roundup(phys_mem_end - pa_start, (1ULL << PGD_SHIFT)) >> PGD_SHIFT;

    int i;
    for (i = 0; i < num_pmds; i++) {
        pgd[(KERNEL_VMA >> PGD_SHIFT) % NUM_DIR_ENTRIES + i] =
            initial_pgd[(KERNEL_VMA >> PGD_SHIFT) % NUM_DIR_ENTRIES + i];
    }

    /* map io regions */
    struct iomap* iom;
    for (iom = iomaps; iom < iomaps + num_iomaps; iom++) {
        vm_mappage(pgd, iom->phys_addr, iom->vir_base,
                   iom->vir_base + iom->size, PROT_KERNEL_EXEC);
    }
}

void* vm_mapio(unsigned long phys_addr, size_t size)
{
    static void* iomap_base = (void*)IOMAP_BASE;

    if (num_iomaps == IOMAP_MAX) {
        panic("too many io mappings");
    }

    unsigned long phys_base = rounddown(phys_addr, PG_SIZE);
    void* vir_addr = iomap_base;
    size_t offset = phys_addr - phys_base;

    size += offset;
    size = roundup(size, PG_SIZE);

    struct iomap* map = &iomaps[num_iomaps++];
    map->vir_base = iomap_base;
    map->phys_addr = phys_base;
    map->size = size;
    printk("iomap: vir 0x%lx-0x%lx -> phys 0x%lx\r\n", map->vir_base,
           map->vir_base + map->size, map->phys_addr);

    vm_mappage(initial_pgd, phys_base, vir_addr, vir_addr + size, PROT_KERNEL);
    flush_tlb();

    iomap_base += size;

    return vir_addr + offset;
}

void switch_address_space(struct proc* p) { write_ptbr(p->vm.ptbr_phys); }
