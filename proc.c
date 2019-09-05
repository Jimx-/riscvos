#include "proc.h"
#include "const.h"
#include "global.h"
#include "proto.h"
#include "vm.h"

#define INIT_ENTRY_POINT PG_SIZE

void Init();

static void spawn_init();

static struct proc idle_proc;

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

    stop_context(&idle_proc);

    restart_local_timer();
    restore_user_context(p);
}

static void spawn_init()
{
    /* setup the two processes */
    extern char _user_text, _user_etext, _user_data, _user_edata;
    struct proc* p = &proc_table[0];

    p->state &= ~PST_FREESLOT;

    p->vm.ptbr_phys = (reg_t)alloc_pages(1);
    p->vm.ptbr_vir = (reg_t*)__va(p->vm.ptbr_phys);
    vm_mapkernel(p);

    /* map user text section */
    unsigned long user_text_start = (unsigned long)__pa(&_user_text);
    unsigned long user_text_end = (unsigned long)__pa(&_user_etext);
    unsigned long user_text_size =
        roundup(user_text_end - user_text_start, PG_SIZE);
    unsigned long user_data_start = (unsigned long)__pa(&_user_data);
    unsigned long user_data_end = (unsigned long)__pa(&_user_edata);
    unsigned long user_data_size =
        roundup(user_data_end - user_data_start, PG_SIZE);

    p->regs.sepc = INIT_ENTRY_POINT + ((char*)&Init - &_user_text);
    p->regs.sp = USER_STACK_TOP;

    INIT_LIST_HEAD(&p->vm.regions);
    vm_map(p, user_text_start, (void*)INIT_ENTRY_POINT,
           (void*)(INIT_ENTRY_POINT + user_text_size));
    vm_map(p, user_data_start, (void*)(INIT_ENTRY_POINT + user_text_size),
           (void*)(INIT_ENTRY_POINT + user_text_size + user_data_size));
    vm_map(p, 0, (void*)(USER_STACK_TOP - USER_STACK_SIZE),
           (void*)USER_STACK_TOP);

    p->quantum_ms = 10;
}
