#include "global.h"
#include "proto.h"
#include "vm.h"

#include <errno.h>
#include <stddef.h>

struct hole {
    struct hole* h_next;
    unsigned long h_base;
    unsigned long h_len;
};

#define NR_HOLES 512

static struct hole hole[NR_HOLES]; /* the hole table */
static struct hole* hole_head;     /* pointer to first hole */
static struct hole* free_slots;    /* ptr to list of unused table slots */

static void delete_slot(struct hole* prev_ptr, struct hole* hp);
static void merge_hole(struct hole* hp);

void mem_init(unsigned long mem_start, unsigned long free_mem_size)
{
    struct hole* hp;

    for (hp = &hole[0]; hp < &hole[NR_HOLES]; hp++) {
        hp->h_next = hp + 1;
        hp->h_base = 0;
        hp->h_len = 0;
    }
    hole[NR_HOLES - 1].h_next = NULL;
    hole_head = NULL;
    free_slots = &hole[0];

    free_mem(mem_start, free_mem_size);
}

unsigned long alloc_mem(unsigned long memsize)
{
    struct hole *hp, *prev_ptr;
    unsigned long old_base;

    prev_ptr = NULL;
    hp = hole_head;

    while (hp != NULL) {
        if (hp->h_len >= memsize) {
            old_base = hp->h_base;
            hp->h_base += memsize;
            hp->h_len -= memsize;

            if (hp->h_len == 0) delete_slot(prev_ptr, hp);

            return old_base;
        }

        prev_ptr = hp;
        hp = hp->h_next;
    }

    return 0;
}

unsigned long alloc_pages(size_t nr_pages)
{
    size_t memsize = nr_pages * PG_SIZE;
    struct hole *hp, *prev_ptr;
    unsigned long old_base;
    unsigned long page_align = PG_SIZE;

    prev_ptr = NULL;
    hp = hole_head;
    while (hp != NULL) {
        size_t alignment = 0;
        if (hp->h_base % page_align != 0)
            alignment = page_align - (hp->h_base % page_align);
        if (hp->h_len >= memsize + alignment) {
            old_base = hp->h_base + alignment;
            hp->h_base += memsize + alignment;
            hp->h_len -= (memsize + alignment);
            if (prev_ptr && prev_ptr->h_base + prev_ptr->h_len == old_base)
                prev_ptr->h_len += alignment;

            if (hp->h_len == 0) delete_slot(prev_ptr, hp);

            return old_base;
        }

        prev_ptr = hp;
        hp = hp->h_next;
    }

    return 0;
}

void* vmalloc_pages(size_t nr_pages, unsigned long* phys_addr)
{
    unsigned long phys = alloc_pages(nr_pages);
    if (!phys) return NULL;

    if (phys_addr) *phys_addr = phys;
    return __va(phys);
}

int free_mem(unsigned long base, unsigned long len)
{
    struct hole *hp, *new_ptr, *prev_ptr;

    if (len == 0) return EINVAL;
    if ((new_ptr = free_slots) == NULL) panic("hole table full");
    new_ptr->h_base = base;
    new_ptr->h_len = len;
    free_slots = new_ptr->h_next;
    hp = hole_head;

    if (hp == NULL || base <= hp->h_base) {
        new_ptr->h_next = hp;
        hole_head = new_ptr;
        merge_hole(new_ptr);
        return 0;
    }

    prev_ptr = NULL;
    while (hp != NULL && base > hp->h_base) {
        prev_ptr = hp;
        hp = hp->h_next;
    }

    new_ptr->h_next = prev_ptr->h_next;
    prev_ptr->h_next = new_ptr;
    merge_hole(prev_ptr);
    return 0;
}

int vmfree(void* ptr, unsigned long len) { return free_mem(__pa(ptr), len); }

static void delete_slot(struct hole* prev_ptr, struct hole* hp)
{
    if (hp == hole_head)
        hole_head = hp->h_next;
    else
        prev_ptr->h_next = hp->h_next;

    hp->h_next = free_slots;
    hp->h_base = hp->h_len = 0;
    free_slots = hp;
}

static void merge_hole(struct hole* hp)
{
    struct hole* next_ptr;

    if ((next_ptr = hp->h_next) == NULL) return;
    if (hp->h_base + hp->h_len == next_ptr->h_base) {
        hp->h_len += next_ptr->h_len;
        delete_slot(hp, next_ptr);
    } else {
        hp = next_ptr;
    }

    if ((next_ptr = hp->h_next) == NULL) return;
    if (hp->h_base + hp->h_len == next_ptr->h_base) {
        hp->h_len += next_ptr->h_len;
        delete_slot(hp, next_ptr);
    }
}
