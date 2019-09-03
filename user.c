#include "const.h"

const char str[] __attribute__((__section__(".user_data"))) = "Hello world!\n";

void Init() __attribute__((__section__(".user_text_entry")));

void __syscall(int call_nr, ...);

/* the INIT process */
void Init()
{
    __syscall(SYS_WRITE_CONSOLE, (unsigned long)str, 13);

    while (1)
        ;
}
