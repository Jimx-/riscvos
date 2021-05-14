#include "hostif.h"
#include "nvme.h"
#include "ssd.h"

#include <assert.h>

static unsigned short aq_sq_depth;
static unsigned short aq_cq_depth;
static uint64_t aq_sq_dma_addr;
static uint64_t aq_cq_dma_addr;

void nvme_process_write_message(uint64_t addr, const char* buf, size_t len)
{
    uint32_t u32;

    switch (addr) {
    case NVME_REG_AQA:
        /* Admin submission queue attribute. */
        assert(len == 4);
        u32 = *(uint32_t*)buf;
        aq_sq_depth = (u32 & 0xfff) + 1;
        aq_cq_depth = ((u32 >> 16) & 0xfff) + 1;
        break;
    case NVME_REG_ASQ:
        /* Admin submission queue base address. */
        assert(len == 8);
        aq_sq_dma_addr = *(uint64_t*)buf;
        break;
    case NVME_REG_ACQ:
        /* Admin completion queue base address. */
        assert(len == 8);
        aq_cq_dma_addr = *(uint64_t*)buf;
        break;
    }
}
