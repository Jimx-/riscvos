#include "const.h"
#include "global.h"
#include "proc.h"
#include "proto.h"

#include <errno.h>
#include <string.h>

int sys_nop() { return ENOSYS; }

static long sys_write_console(struct proc* p, const char* str, int len)
{
    char buf[256];
    copy_from_user(buf, str, len);
    buf[len] = '\0';
    direct_put_str(buf);
    return 0;
}

static long sys_fork(struct proc* p)
{
    int i;
    struct proc *pp = proc_table, *child;
    struct reg_context *parent_context, *child_context;
    int pid;

    /* find an empty slot in the proc table */
    for (i = 0; i < PROC_MAX; i++, pp++) {
        if (pp->state & PST_FREESLOT) {
            pp->state &= ~PST_FREESLOT;
            break;
        }
    }

    if (i >= PROC_MAX) return -ENOMEM;
    pid = i;
    child = &proc_table[pid];

    /* allocate page directory for the child and set up kernel mappings */
    child->vm.ptbr_phys = (reg_t)alloc_pages(1);
    child->vm.ptbr_vir = (reg_t*)__va(child->vm.ptbr_phys);
    vm_mapkernel(child);

    child->kernel_stack = __va(alloc_pages(1));
    parent_context = (struct reg_context*)(p->kernel_stack + PG_SIZE -
                                           sizeof(struct reg_context));
    child_context = (struct reg_context*)(child->kernel_stack + PG_SIZE -
                                          sizeof(struct reg_context));
    child->regs.ra = (reg_t)&restore_user_context;
    child->regs.sp = (reg_t)child_context;

    INIT_LIST_HEAD(&child->vm.regions);
    struct vm_region* vmr;
    list_for_each_entry(vmr, &p->vm.regions, list)
    {
        /* allocate physical memory for the region */
        unsigned long new_phys = alloc_pages(vmr->size >> PG_SHIFT);

        /* copy the content to the new region */
        memcpy(__va(new_phys), __va(vmr->phys_base), vmr->size);

        /* map the new physical region into child's address space */
        vm_map(child, new_phys, vmr->vir_base, vmr->vir_base + vmr->size);
    }

    child->quantum_ms = p->quantum_ms;

    /* return 0 to child process */
    *child_context = *parent_context;
    child_context->a0 = 0;

    return pid;
}

static long sys_open(struct proc* p, const char* name, unsigned int name_len, int flags, int mode) {
    int fd = -1;
    char pathname[PATH_MAX + 1];

    if (name_len > PATH_MAX) {
        return -ENAMETOOLONG;
    }

    copy_from_user(pathname, name, name_len);
    pathname[name_len] = 0;

    /* ... */

    return fd;
}

void* syscall_table[NR_SYSCALLS] = {
    [SYS_WRITE_CONSOLE] = sys_write_console,
    [SYS_FORK] = sys_fork,
    [SYS_OPEN] = sys_open,
};
