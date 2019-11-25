#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef GLOBAL_VARIABLES_HERE
#define EXTERN
#else
#define EXTERN extern
#endif

#include "proc.h"

EXTERN unsigned long va_pa_offset;
EXTERN unsigned long phys_mem_end;

EXTERN int dt_root_addr_cells, dt_root_size_cells;

EXTERN struct proc* current;
EXTERN struct proc proc_table[PROC_MAX];

#endif
