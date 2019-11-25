#ifndef _PROTO_H_
#define _PROTO_H_

#include "proc.h"

#include <stddef.h>
#include <stdint.h>

/* directy_tty.c */
void disp_char(const char c);
void direct_put_str(const char* str);
int printk(const char* fmt, ...);
void panic(const char* fmt, ...);

/* memory.c */
void init_memory(void* dtb);
void* alloc_page(unsigned long* phys_addr);
void copy_from_user(void* dst, const void* src, size_t len);

/* vm.c */
void vm_map(struct proc* p, unsigned long phys_addr, void* vir_addr,
            void* vir_end);
void vm_mapkernel(struct proc* p);
void* vm_mapio(unsigned long phys_addr, size_t size);

/* proc.c */
void init_proc();
struct proc* pick_proc();
void switch_to_user();

/* exc.c */
void init_trap();

/* trap.S */
void switch_context(struct proc* prev, struct proc* next);
void restore_user_context();

void switch_address_space(struct proc* p);

/* clock.c */
void init_timer(void* dtb);
uint64_t read_cycles();
void restart_local_timer();
void timer_interrupt();
void stop_context(struct proc* p);

/* alloc.c */
void mem_init(unsigned long mem_start, unsigned long free_mem_size);
unsigned long alloc_pages(size_t nr_pages);
int free_mem(unsigned long base, unsigned long len);

/* slab.c */
void slabs_init();
void* slaballoc(size_t bytes);
void slabfree(void* mem, size_t bytes);
#define SLABALLOC(p)               \
    do {                           \
        p = slaballoc(sizeof(*p)); \
    } while (0)
#define SLABFREE(p)              \
    do {                         \
        slabfree(p, sizeof(*p)); \
        p = NULL;                \
    } while (0)

/* blk.c */
int init_blkdev();
int blk_rdwt(int write, unsigned int block_num, size_t count, uint8_t* buf);

#endif
