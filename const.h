#ifndef _CONST_H_
#define _CONST_H_

#define KSTACK_SIZE 0x1000 /* kernel stack size 4kB */

#ifndef __ASSEMBLY__

#define roundup(x, align) \
    (((x) % align == 0) ? (x) : (((x) + align) - ((x) % align)))
#define rounddown(x, align) ((x) - ((x) % align))

#endif

#endif
