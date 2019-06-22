#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef GLOBAL_VARIABLES_HERE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN unsigned long va_pa_offset;

#endif
