#ifndef _CONST_H_
#define _CONST_H_

#define KSTACK_SIZE 0x1000 /* kernel stack size 4kB */

/* syscall numbers */
#define NR_SYSCALLS 2
#define SYS_WRITE_CONSOLE 0 /* write a string to console */
#define SYS_FORK 1          /* fork */

#ifndef __ASSEMBLY__

#define roundup(x, align) \
    (((x) % align == 0) ? (x) : (((x) + align) - ((x) % align)))
#define rounddown(x, align) ((x) - ((x) % align))

#endif

#define SYSTEM_HZ 100

#endif
