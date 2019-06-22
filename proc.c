#include "proc.h"
#include "const.h"
#include "global.h"
#include "proto.h"
#include "vm.h"

#define INIT_ENTRY_POINT PG_SIZE

static void spawn_init();

void init_proc()
{
    int i;
    for (i = 0; i < PROC_MAX; i++) {
        struct proc* p = &proc_table[i];

        p->state = PST_FREESLOT;
    }

    spawn_init();
}

void switch_to_user()
{
    struct proc* p;
    extern char KStackTop;

    p = pick_proc();

    /* set kernel stack */
    p->regs.kernel_sp = (reg_t)&KStackTop;
    switch_address_space(p);

    restore_user_context(p);
}

static void spawn_init()
{
    /* setup everything for the INIT process */
    extern char _user_text, _user_etext;
    struct proc* p = &proc_table[0];

    p->state &= ~PST_FREESLOT;

    /* reuse the initial page table */
    p->segs.ptbr_phys = (reg_t)__pa(initial_pgd);
    p->segs.ptbr_vir = (reg_t*)initial_pgd;

    /* map user text section */
    unsigned long user_text_start = (unsigned long)__pa(&_user_text);
    unsigned long user_text_end = (unsigned long)__pa(&_user_etext);
    unsigned long user_text_size =
        roundup(user_text_end - user_text_start, PG_SIZE);

    vm_map(p, user_text_start, (void*)INIT_ENTRY_POINT,
           (void*)(INIT_ENTRY_POINT + user_text_size));
    p->regs.sepc = INIT_ENTRY_POINT;

    /* allocate stack */
    vm_map(p, 0, (void*)(KERNEL_VMA - PG_SIZE), (void*)KERNEL_VMA);
    p->regs.sp = KERNEL_VMA;
}
