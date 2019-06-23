#ifndef _PROTO_H_
#define _PROTO_H_

#include "proc.h"

#include <stddef.h>
#include <stdint.h>

/* directy_tty.c */
void disp_char(const char c);
void direct_put_str(const char* str);
int printk(const char* fmt, ...);

/* memory.c */
void init_memory(void* dtb);
void* alloc_page(unsigned long* phys_addr);
void copy_from_user(void* dst, const void* src, size_t len);

/* vm.c */
void vm_map(struct proc* p, unsigned long phys_addr, void* vir_addr,
            void* vir_end);

/* proc.c */
void init_proc();
struct proc* pick_proc();
void switch_to_user();

/* exc.c */
void init_trap();

void restore_user_context(struct proc* p);
void switch_address_space(struct proc* p);

/* clock.c */
uint64_t read_cycles();
void restart_local_timer();
void timer_interrupt();

#endif
