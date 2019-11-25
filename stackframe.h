#ifndef _STACKFRAME_H_
#define _STACKFRAME_H_

typedef unsigned long reg_t;

struct reg_context {
    /* save user registers in the frame upon context switch */
    reg_t sepc;
    reg_t ra;
    reg_t sp;
    reg_t gp;
    reg_t tp;
    reg_t t0;
    reg_t t1;
    reg_t t2;
    reg_t s0;
    reg_t s1;
    reg_t a0;
    reg_t a1;
    reg_t a2;
    reg_t a3;
    reg_t a4;
    reg_t a5;
    reg_t a6;
    reg_t a7;
    reg_t s2;
    reg_t s3;
    reg_t s4;
    reg_t s5;
    reg_t s6;
    reg_t s7;
    reg_t s8;
    reg_t s9;
    reg_t s10;
    reg_t s11;
    reg_t t3;
    reg_t t4;
    reg_t t5;
    reg_t t6;
    /* Supervisor CSRs */
    reg_t sstatus;
    reg_t sbadaddr;
    reg_t scause;

    reg_t orig_a0;
};

struct thread_info {
    reg_t kernel_sp;
    reg_t user_sp;
};

#endif
