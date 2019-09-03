#ifndef _BITMAP_H_
#define _BITMAP_H_

typedef unsigned long bitchunk_t;

#define CHAR_BITS 8
#define BITCHUNK_BITS (sizeof(bitchunk_t) * CHAR_BITS)
#define BITCHUNKS(bits) (((bits) + BITCHUNK_BITS - 1) / BITCHUNK_BITS)

#define MAP_CHUNK(map, bit) (map)[((bit) / BITCHUNK_BITS)]
#define CHUNK_OFFSET(bit) ((bit) % BITCHUNK_BITS)
#define GET_BIT(map, bit) (MAP_CHUNK(map, bit) & (1 << CHUNK_OFFSET(bit)))
#define SET_BIT(map, bit) (MAP_CHUNK(map, bit) |= (1 << CHUNK_OFFSET(bit)))
#define UNSET_BIT(map, bit) (MAP_CHUNK(map, bit) &= ~(1 << CHUNK_OFFSET(bit)))

#endif
