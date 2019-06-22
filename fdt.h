#ifndef _FDT_H_
#define _FDT_H_

#include "byteorder.h"
#include "libfdt.h"

static inline uint64_t of_read_number(const uint32_t* cell, int size)
{
    uint64_t r = 0;
    while (size--) {
        r = (r << 32) | be32_to_cpup((uint32_t*)cell++);
    }
    return r;
}

int of_scan_fdt(int (*scan)(void*, unsigned long, const char*, int, void*),
                void* arg, void* blob);

#endif
