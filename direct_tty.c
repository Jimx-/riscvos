#include "sbi.h"

void disp_char(const char c) { sbi_console_putchar((int)c); }

void direct_put_str(const char* str)
{
    while (*str) {
        disp_char(*str);
        str++;
    }
}
