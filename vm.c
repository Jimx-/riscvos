#include "vm.h"
#include "global.h"
#include "stdint.h" /* for uintptr_t */

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
