#include "const.h"

void TestA() __attribute__((__section__(".user_text")));
void TestB() __attribute__((__section__(".user_text")));

void __syscall(int call_nr, ...);

void TestA()
{
    const char str[] = "A";
    while (1) {
        __syscall(SYS_WRITE_CONSOLE, (unsigned long)str, 1);
    }
}

void TestB()
{
    const char str[] = "B";
    while (1) {
        __syscall(SYS_WRITE_CONSOLE, (unsigned long)str, 1);
    }
}
