    #include "const.h"
    #include "csr.h"
    #include "vm.h"

.section .head

.globl _start	/* entry point */

_start:
    /* mask interrupts */
    csrw sie, zero

.option push
.option norelax
    /* load global pointer */
    la gp, __global_pointer$
.option pop

    /* save hart ID and DTB from bootloader */
    mv s0, a0
    mv s1, a1

    /* setup stack pointer */
    la sp, KStackTop

    /* setup initial page table */
    call setup_paging
    call enable_paging

    /* reload stack pointer */
    la sp, KStackTop

    mv a0, s0
    mv a1, s1
    tail kernel_main

enable_paging:
    /* relocate return address(adding the va pa offset) */
    li a1, KERNEL_VMA
    la a0, _start
    sub a1, a1, a0
    add ra, ra, a1

    /* load stvec with the virtual address of 1f */
    /* because the initial page table is not an identity mapping, the cpu will
     * trigger a page fault trying to fetch the next instruction once vm is enabled,
     * which causes the cpu to jump to the virtual address of 1f */
    la a0, 1f
    add a0, a0, a1
    csrw stvec, a0

    /* computer the SATP for initial pgd */
    la a2, initial_pgd
    srl a2, a2, PG_SHIFT
    li a1, SATP_MODE
    or a2, a2, a1

    sfence.vma
    csrw sptbr, a2

.align 2
1:
    /* reload global pointer */
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    ret

.section .data
KStackSpace:
    .zero KSTACK_SIZE
KStackTop:
