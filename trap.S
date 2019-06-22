    #include "csr.h"
    #include "reg_offsets.h"

    .section .text

    .globl trap_entry
    .globl restore_user_context

.macro test_in_kernel   label
    /* determine whether we were in kernel or userspace before the trap */
    csrr s0, sstatus
    andi s0, s0, SR_SPP
    bnez \label
.endm

.macro save_context
    /* save all registers in proc struct, tp is the pointer to current proc */
    /* save user sp and restore kernel sp */
    sd sp, SPREG(tp)
    ld sp, KERNELSPREG(tp)

    /* save all general registers */
    sd x1,  RAREG(tp)
    sd x3,  GPREG(tp)
    sd x5,  T0REG(tp)
    sd x6,  T1REG(tp)
    sd x7,  T2REG(tp)
    sd x8,  S0REG(tp)
    sd x9,  S1REG(tp)
    sd x10, A0REG(tp)
    sd x11, A1REG(tp)
    sd x12, A2REG(tp)
    sd x13, A3REG(tp)
    sd x14, A4REG(tp)
    sd x15, A5REG(tp)
    sd x16, A6REG(tp)
    sd x17, A7REG(tp)
    sd x18, S2REG(tp)
    sd x19, S3REG(tp)
    sd x20, S4REG(tp)
    sd x21, S5REG(tp)
    sd x22, S6REG(tp)
    sd x23, S7REG(tp)
    sd x24, S8REG(tp)
    sd x25, S9REG(tp)
    sd x26, S10REG(tp)
    sd x27, S11REG(tp)
    sd x28, T3REG(tp)
    sd x29, T4REG(tp)
    sd x30, T5REG(tp)
    sd x31, T6REG(tp)

    /* disable user memory access */
    li t0, SR_SUM
    csrrc s0, sstatus, t0

    csrr s1, sepc
    csrr s2, sbadaddr
    csrr s3, scause
    csrr s4, sscratch

    sd s0, SSTATUSREG(tp)
    sd s1, SEPCREG(tp)
    sd s2, SBADADDRREG(tp)
    sd s3, SCAUSEREG(tp)
    sd s4, TPREG(tp)
.endm

.align 4
trap_entry:
    /* sscratch == 0 if the call is from kernel, otherwise sscratch is a pointer to
     * the calling proc */
    csrrw tp, sscratch, tp
    beqz tp, trap_entry_kernel

    save_context

    /* clear sscratch as we are already in kernel */
    csrw sscratch, x0

.option push
.option norelax
    /* load kernel global pointer */
    la gp, __global_pointer$
.option pop
    
    /* check whether cause is interrupt or exception */
    bge s3, zero, do_exception /* s3 was set to scause in save_context */

do_irq:
    tail switch_to_user

do_exception:
    li t0, EXC_SYSCALL
    beq s3, t0, do_syscall

    ld a0, SSTATUSREG(tp)
    andi a0, a0, SR_SPP     /* in_kernel */
    mv a1, tp               /* proc_ptr */

    slli t0, s3, 3
    la t1, exc_vector_table
    la t2, exc_vector_table_end
    add t0, t1, t0
    bgeu t1, t2, 1f

    ld t0, 0(t0)
    jalr t0

    tail switch_to_user
    
1:
    call do_trap_unknown
    tail switch_to_user

do_syscall:
    sd a0, P_ORIGA0(tp)

    /* relocate the return pc(skip the original scall instruction) */
    addi s1, s1, 0x4
    sd s1, SEPCREG(tp)

    /* a1 is the call_nr passed by user */
    mv a2, tp
    call handle_sys_call

    tail switch_to_user

trap_entry_kernel:
    csrr tp, sscratch
    j trap_entry_kernel

restore_user_context:
    mv tp, a0
	ld s0, SSTATUSREG(tp)
	andi s0, s0, SR_SPP
	bnez s0, restore_all

    /* save tp in sscratch only if we are returning to userspace */
    csrw sscratch, tp

restore_all:
    ld a0, SSTATUSREG(tp)
    ld a1, SEPCREG(tp)
    csrw sstatus, a0
    csrw sepc, a1

	ld x1,  RAREG(tp)
	ld x2,  SPREG(tp)
	ld x3,  GPREG(tp)
	ld x5,  T0REG(tp)
	ld x6,  T1REG(tp)
	ld x7,  T2REG(tp)
	ld x8,  S0REG(tp)
	ld x9,  S1REG(tp)
	ld x10, A0REG(tp)
	ld x11, A1REG(tp)
	ld x12, A2REG(tp)
	ld x13, A3REG(tp)
	ld x14, A4REG(tp)
	ld x15, A5REG(tp)
	ld x16, A6REG(tp)
	ld x17, A7REG(tp)
	ld x18, S2REG(tp)
	ld x19, S3REG(tp)
	ld x20, S4REG(tp)
	ld x21, S5REG(tp)
	ld x22, S6REG(tp)
	ld x23, S7REG(tp)
	ld x24, S8REG(tp)
	ld x25, S9REG(tp)
	ld x26, S10REG(tp)
	ld x27, S11REG(tp)
	ld x28, T3REG(tp)
	ld x29, T4REG(tp)
	ld x30, T5REG(tp)
	ld x31, T6REG(tp)

	ld x4,  TPREG(tp)

    sret

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
