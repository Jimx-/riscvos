    #include "const.h"
    #include "csr.h"
    #include "vm.h"
    #include "config.h"

    .section ".head","ax"

    .globl _start/* entry point */
    .globl KStackTop /* kernel stack */

_start:
    /* mask interrupts */
    csrw sie, zero

.option push
.option norelax
    /* load global pointer */
    la gp, __global_pointer$
.option pop

    /* pick one hart as the BSP */
    la a3, hart_counter
    li a2, 1
    amoadd.w a3, a2, (a3)
    bnez a3, .Lsecondary_start

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
    mv tp, x0

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
    csrw satp, a2
.align 2
1:
    la a1, .Lno_smp
    csrw stvec, a1

    /* reload global pointer */
.option push
.option norelax
    la gp, __global_pointer$
.option pop

    ret

.Lsecondary_start:
    li a1, CONFIG_SMP_MAX_CPUS
    bgeu a0, a1, .Lno_smp

    la a3, .Lno_smp
    csrw stvec, a3

    slli a3, a0, 3
    la a1, __cpu_stack_pointer
    la a2, __cpu_task_pointer
    add a1, a1, a3
    add a2, a2, a3

.Lap_wait:
    ld sp, (a1)
    ld tp, (a2)
    beqz sp, .Lap_wait
    beqz tp, .Lap_wait

    fence

    call enable_paging
    tail smp_boot_ap

.align 2
.Lno_smp:
    wfi
    j .Lno_smp

.section .data
KStackSpace:
    .zero KSTACK_SIZE
KStackTop:

.globl k_stacks_start, k_stacks_end
.balign	KSTACK_SIZE
k_stacks_start:
    .space 2 * KSTACK_SIZE * CONFIG_SMP_MAX_CPUS
k_stacks_end:
