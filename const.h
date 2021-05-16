#ifndef _CONST_H_
#define _CONST_H_

#define KSTACK_SIZE 0x8000 /* kernel stack size 4kB */

/* syscall numbers */
#define NR_SYSCALLS 3
#define SYS_WRITE_CONSOLE 0 /* write a string to console */
#define SYS_FORK 1          /* fork */
#define SYS_OPEN 2          /* open a file */

#ifndef __ASSEMBLY__

#define roundup(x, align) \
    (((x) % align == 0) ? (x) : (((x) + align) - ((x) % align)))
#define rounddown(x, align) ((x) - ((x) % align))

#endif

#define SYSTEM_HZ 100

#define BLOCK_SIZE 0x1000

#define PATH_MAX 256

#define VSOCK_HOST_CID 2
#define VSOCK_HOST_PORT 9999

#endif
