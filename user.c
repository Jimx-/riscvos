#include "const.h"

const char pstr[] __attribute__((__section__(".user_data"))) = "parent\r\n";
const char cstr[] __attribute__((__section__(".user_data"))) = "child\r\n";

void Init() __attribute__((__section__(".user_text")));

long __syscall(int call_nr, ...);
static void write_console(const char* str)
    __attribute__((__section__(".user_text")));
static long fork(void) __attribute__((__section__(".user_text")));

void Init()
{
    int pid = fork();

    if (pid > 0) {
        write_console(pstr);
    } else {
        write_console(cstr);
    }

    while (1) {
    }
}

static void write_console(const char* str)
{
    int len = 0;
    const char* p = str;
    while (*p++)
        len++;

    __syscall(SYS_WRITE_CONSOLE, (unsigned long)str, len);
}

static long fork(void) { return __syscall(SYS_FORK); }
