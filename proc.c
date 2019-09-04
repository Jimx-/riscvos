#include "proc.h"
#include "const.h"
#include "global.h"
#include "proto.h"
#include "vm.h"

#define INIT_ENTRY_POINT PG_SIZE

void TestA();
void TestB();

static void spawn_procs();

static struct proc idle_proc;

void init_proc()
{
    int i;
    for (i = 0; i < PROC_MAX; i++) {
        struct proc* p = &proc_table[i];

        p->state = PST_FREESLOT;
    }

    spawn_procs();
}

void switch_to_user()
{
    struct proc* p;
    extern char KStackTop;

    p = pick_proc();

    /* set kernel stack */
    p->regs.kernel_sp = (reg_t)&KStackTop;
    switch_address_space(p);

    stop_context(&idle_proc);

    restart_local_timer();
    restore_user_context(p);
}

static void spawn_procs()
{
    /* setup the two processes */
    extern char _user_text, _user_etext, _user_data, _user_edata;
    struct proc *pa = &proc_table[0], *pb = &proc_table[1];

    pa->state &= ~PST_FREESLOT;
    pb->state &= ~PST_FREESLOT;

    /* reuse the initial page table */
    pa->vm.ptbr_phys = (reg_t)alloc_pages(1);
    pa->vm.ptbr_vir = (reg_t*)__va(pa->vm.ptbr_phys);
    vm_mapkernel(pa);

    pb->vm.ptbr_phys = (reg_t)alloc_pages(1);
    pb->vm.ptbr_vir = (reg_t*)__va(pb->vm.ptbr_phys);
    vm_mapkernel(pb);

    /* map user text section */
    unsigned long user_text_start = (unsigned long)__pa(&_user_text);
    unsigned long user_text_end = (unsigned long)__pa(&_user_etext);
    unsigned long user_text_size =
        roundup(user_text_end - user_text_start, PG_SIZE);
    unsigned long user_data_start = (unsigned long)__pa(&_user_data);
    unsigned long user_data_end = (unsigned long)__pa(&_user_edata);
    unsigned long user_data_size =
        roundup(user_data_end - user_data_start, PG_SIZE);

    pa->regs.sepc = INIT_ENTRY_POINT + ((char*)&TestA - &_user_text);
    pa->regs.sp = USER_STACK_TOP;
    pb->regs.sepc = INIT_ENTRY_POINT + ((char*)&TestB - &_user_text);
    pb->regs.sp = USER_STACK_TOP;

    vm_map(pa, user_text_start, (void*)INIT_ENTRY_POINT,
           (void*)(INIT_ENTRY_POINT + user_text_size));
    vm_map(pa, user_data_start, (void*)(INIT_ENTRY_POINT + user_text_size),
           (void*)(INIT_ENTRY_POINT + user_text_size + user_data_size));
    vm_map(pa, 0, (void*)(USER_STACK_TOP - USER_STACK_SIZE),
           (void*)USER_STACK_TOP);

    vm_map(pb, user_text_start, (void*)INIT_ENTRY_POINT,
           (void*)(INIT_ENTRY_POINT + user_text_size));
    vm_map(pb, user_data_start, (void*)(INIT_ENTRY_POINT + user_text_size),
           (void*)(INIT_ENTRY_POINT + user_text_size + user_data_size));
    vm_map(pb, 0, (void*)(USER_STACK_TOP - USER_STACK_SIZE),
           (void*)USER_STACK_TOP);

    pa->quantum_ms = 10;
    pb->quantum_ms = 20;
}
