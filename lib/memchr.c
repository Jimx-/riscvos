#include <stddef.h>

void* memchr(register const void* src_void, int c, size_t length)
{
    const unsigned char* src = (const unsigned char*)src_void;

    while (length-- > 0) {
        if (*src == c) return (void*)src;
        src++;
    }
    return NULL;
}
