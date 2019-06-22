#ifndef _PROTO_H_
#define _PROTO_H_

void disp_char(const char c);
void direct_put_str(const char* str);
int printk(const char* fmt, ...);

void init_memory(void* dtb);

#endif
