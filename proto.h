#ifndef _PROTO_H_
#define _PROTO_H_

#include "proc.h"

void disp_char(const char c);
void direct_put_str(const char* str);
int printk(const char* fmt, ...);

void init_memory(void* dtb);
void* alloc_page(unsigned long* phys_addr);

void vm_map(struct proc* p, unsigned long phys_addr, void* vir_addr, void* vir_end);

void init_proc();
struct proc* pick_proc();
void switch_to_user();

void init_trap();

void restore_user_context(struct proc* p);
void switch_address_space(struct proc* p);

#endif
