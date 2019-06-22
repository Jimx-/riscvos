#ifndef _CONST_H_
#define _CONST_H_

#define KSTACK_SIZE 0x1000 /* kernel stack size 4kB */

/* syscall numbers */
#define NR_SYSCALLS 1
#define SYS_WRITE_CONSOLE 0 /* write a string to console */

#ifndef __ASSEMBLY__

#define roundup(x, align) \
    (((x) % align == 0) ? (x) : (((x) + align) - ((x) % align)))
#define rounddown(x, align) ((x) - ((x) % align))

#endif

#endif
