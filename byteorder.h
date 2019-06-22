#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

#include <stdint.h>

static inline uint32_t __swab32(uint32_t x)
{
    return ((uint32_t)(((x & (uint32_t)0x000000ffUL) << 24) |
                       ((x & (uint32_t)0x0000ff00UL) << 8) |
                       ((x & (uint32_t)0x00ff0000UL) >> 8) |
                       ((x & (uint32_t)0xff000000UL) >> 24)));
}

static inline uint32_t __swab32p(const uint32_t* p) { return __swab32(*p); }

static inline uint32_t be32_to_cpup(const uint32_t* p)
{
    return (uint32_t)__swab32p(p);
}

#endif
