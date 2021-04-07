    #include "csr.h"
    #include "const.h"
    #include "reg_offsets.h"

    .section .text

    .globl trap_entry
    .globl switch_context
    .globl restore_user_context

.macro save_context
    addi sp, sp, -(P_FRAMESIZE)

    /* save all general registers */
    sd x1,  RAREG(sp)
    sd x3,  GPREG(sp)
    sd x5,  T0REG(sp)
    sd x6,  T1REG(sp)
    sd x7,  T2REG(sp)
    sd x8,  S0REG(sp)
    sd x9,  S1REG(sp)
    sd x10, A0REG(sp)
    sd x11, A1REG(sp)
    sd x12, A2REG(sp)
    sd x13, A3REG(sp)
    sd x14, A4REG(sp)
    sd x15, A5REG(sp)
    sd x16, A6REG(sp)
    sd x17, A7REG(sp)
    sd x18, S2REG(sp)
    sd x19, S3REG(sp)
    sd x20, S4REG(sp)
    sd x21, S5REG(sp)
    sd x22, S6REG(sp)
    sd x23, S7REG(sp)
    sd x24, S8REG(sp)
    sd x25, S9REG(sp)
    sd x26, S10REG(sp)
    sd x27, S11REG(sp)
    sd x28, T3REG(sp)
    sd x29, T4REG(sp)
    sd x30, T5REG(sp)
    sd x31, T6REG(sp)

    li t0, SR_SUM
    csrrc s0, sstatus, t0

    addi s7, sp, P_FRAMESIZE
    csrr s8, sepc
    csrr s9, sbadaddr
    csrr s10, scause
    csrr s11, sscratch

    sd s0, SSTATUSREG(sp)
    sd s7, SPREG(sp)
    sd s8, SEPCREG(sp)
    sd s9, SBADADDRREG(sp)
    sd s10, SCAUSEREG(sp)
    sd s11, TPREG(sp)
.endm

.align 4
trap_entry:
    save_context

    /* clear sscratch as we are already in kernel */
    csrw sscratch, x0

.option push
.option norelax
    /* load kernel global pointer */
    la gp, __global_pointer$
.option pop

    /* check whether cause is interrupt or exception */
    bge s10, zero, do_exception /* s10 was set to scause in save_context */

do_irq:
    mv a0, s10
    call handle_irq

    j restore_all

do_exception:
    ld a0, SSTATUSREG(sp)
    andi a0, a0, SR_SPP     /* in_kernel */
    mv a1, s9               /* sbadaddr */
    mv a2, s8               /* sepc */

    slli t0, s10, 3
    la t1, exc_vector_table
    la t2, exc_vector_table_end
    add t0, t1, t0
    bgeu t1, t2, 1f

    ld t0, 0(t0)
    jalr t0

    j restore_all

1:
    call do_trap_unknown
    j restore_all

restore_user_context:
    ld s0, SSTATUSREG(tp)
    andi s0, s0, SR_SPP
    bnez s0, restore_all

    addi s0, sp, (P_FRAMESIZE)
    sd s0, P_KERNEL_SP(tp)

    /* save tp in sscratch only if we are returning to userspace */
    csrw sscratch, tp

restore_all:
    ld a0, SSTATUSREG(sp)
    ld a1, SEPCREG(sp)
    csrw sstatus, a0
    csrw sepc, a1

    ld x1,  RAREG(sp)
    ld x3,  GPREG(sp)
    ld x5,  T0REG(sp)
    ld x6,  T1REG(sp)
    ld x7,  T2REG(sp)
    ld x8,  S0REG(sp)
    ld x9,  S1REG(sp)
    ld x10, A0REG(sp)
    ld x11, A1REG(sp)
    ld x12, A2REG(sp)
    ld x13, A3REG(sp)
    ld x14, A4REG(sp)
    ld x15, A5REG(sp)
    ld x16, A6REG(sp)
    ld x17, A7REG(sp)
    ld x18, S2REG(sp)
    ld x19, S3REG(sp)
    ld x20, S4REG(sp)
    ld x21, S5REG(sp)
    ld x22, S6REG(sp)
    ld x23, S7REG(sp)
    ld x24, S8REG(sp)
    ld x25, S9REG(sp)
    ld x26, S10REG(sp)
    ld x27, S11REG(sp)
    ld x28, T3REG(sp)
    ld x29, T4REG(sp)
    ld x30, T5REG(sp)
    ld x31, T6REG(sp)

    ld x4,  TPREG(sp)
    ld sp,  SPREG(sp)

    sret

switch_context:
    sd x1,  RAREG(a0)
    sd x2,  SPREG(a0)
    sd x8,  S0REG(a0)
    sd x9,  S1REG(a0)
    sd x18, S2REG(a0)
    sd x19, S3REG(a0)
    sd x20, S4REG(a0)
    sd x21, S5REG(a0)
    sd x22, S6REG(a0)
    sd x23, S7REG(a0)
    sd x24, S8REG(a0)
    sd x25, S9REG(a0)
    sd x26, S10REG(a0)
    sd x27, S11REG(a0)

    ld x1,  RAREG(a1)
    ld x2,  SPREG(a1)
    ld x8,  S0REG(a1)
    ld x9,  S1REG(a1)
    ld x18, S2REG(a1)
    ld x19, S3REG(a1)
    ld x20, S4REG(a1)
    ld x21, S5REG(a1)
    ld x22, S6REG(a1)
    ld x23, S7REG(a1)
    ld x24, S8REG(a1)
    ld x25, S9REG(a1)
    ld x26, S10REG(a1)
    ld x27, S11REG(a1)

    move tp, a1
    ret

exc_vector_table:
    .dword do_trap_insn_misaligned
    .dword do_trap_insn_fault
    .dword do_trap_insn_illegal
    .dword do_trap_break
    .dword do_trap_load_misaligned
    .dword do_trap_load_fault
    .dword do_trap_store_misaligned
    .dword do_trap_store_fault
    .dword do_trap_ecall_u
    .dword do_trap_ecall_s
    .dword do_trap_unknown
    .dword do_trap_ecall_m
    .dword do_page_fault
    .dword do_page_fault
    .dword do_trap_unknown
    .dword do_page_fault
exc_vector_table_end:
