#include "sbi.h"

#include <stdarg.h> /* for va_list */

int vsprintf(char* buf, const char* fmt, va_list args);

void disp_char(const char c) { sbi_console_putchar((int)c); }

void direct_put_str(const char* str)
{
    while (*str) {
        disp_char(*str);
        str++;
    }
}

int printk(const char* fmt, ...)
{
    int i;
    char buf[256];
    va_list arg;

    va_start(arg, fmt);
    i = vsprintf(buf, fmt, arg);
    direct_put_str(buf);

    va_end(arg);

    return i;
}

void panic(const char* fmt, ...)
{
    char buf[256];
    va_list arg;

    va_start(arg, fmt);
    vsprintf(buf, fmt, arg);
    va_end(arg);

    printk("Kernel panic: %s\n", buf);

    while (1)
        ;
}
