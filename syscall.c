#include "const.h"
#include "proc.h"
#include "proto.h"

#include <errno.h>

int sys_nop() { return ENOSYS; }

static int sys_write_console(struct proc* p, const char* str, int len)
{
    char buf[256];
    copy_from_user(buf, str, len);
    buf[len] = '\0';
    direct_put_str(buf);
    return 0;
}

void* syscall_table[NR_SYSCALLS] = {
    [SYS_WRITE_CONSOLE] = sys_write_console,
};
