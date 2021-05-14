#ifndef _SSD_HOSTIF_H_
#define _SSD_HOSTIF_H_

#include <stddef.h>
#include <stdint.h>

void hostif_init(void);

/* hostif.c */
void nvme_process_write_message(uint64_t addr, const char* buf, size_t len);

#endif
