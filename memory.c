#include "const.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"
#include "vm.h"

#include <stdint.h>

#define MEMMAP_MAX 10
static struct memmap_entry {
    unsigned long base, size;
} memmaps[MEMMAP_MAX];
static int memmap_count;

static void cut_memmap(unsigned long start, unsigned long end)
{
    int i;

    for (i = 0; i < memmap_count; i++) {
        struct memmap_entry* entry = &memmaps[i];
        unsigned long mmap_end = entry->base + entry->size;
        if (start >= entry->base && end <= mmap_end) {
            entry->base = end;
            entry->size = mmap_end - entry->base;
        }
    }
}

static int fdt_scan_root(void* blob, unsigned long offset, const char* name,
                         int depth, void* arg)
{
    if (depth != 0) return 0;

    dt_root_addr_cells = 1;
    dt_root_size_cells = 1;

    const uint32_t* prop;
    if ((prop = fdt_getprop(blob, offset, "#size-cells", NULL)) != NULL) {
        dt_root_size_cells = be32_to_cpup(prop);
    }

    if ((prop = fdt_getprop(blob, offset, "#address-cells", NULL)) != NULL) {
        dt_root_addr_cells = be32_to_cpup(prop);
    }

    return 1;
}

static int fdt_scan_memory(void* blob, unsigned long offset, const char* name,
                           int depth, void* arg)
{
    extern char _start, _end;
    const char* type = fdt_getprop(blob, offset, "device_type", NULL);
    if (!type || strcmp(type, "memory") != 0) return 0;

    const uint32_t *reg, *lim;
    int len;
    reg = fdt_getprop(blob, offset, "reg", &len);
    if (!reg) return 0;

    lim = reg + (len / sizeof(uint32_t));

    while ((int)(lim - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
        uint64_t base, size;

        base = of_read_number(reg, dt_root_addr_cells);
        reg += dt_root_addr_cells;
        size = of_read_number(reg, dt_root_size_cells);
        reg += dt_root_size_cells;

        if (size == 0) continue;

        memmaps[memmap_count].base = base;
        memmaps[memmap_count].size = size;
        memmap_count++;
    }

    cut_memmap((unsigned long)__pa(&_start),
               roundup((unsigned long)__pa(&_end), PG_SIZE));

    unsigned long memory_size = 0;
    phys_mem_end = 0;
    printk("Physical RAM map:\r\n");
    int i;
    int first = 1;
    for (i = 0; i < memmap_count; i++) {
        struct memmap_entry* entry = &memmaps[i];
        memory_size += entry->size;

        if (first) {
            mem_init(entry->base, entry->size);
            first = 0;
        } else {
            free_mem(entry->base, entry->size);
        }

        if (entry->base + entry->size > phys_mem_end) {
            phys_mem_end = entry->base + entry->size;
        }

        printk("  mem[0x%016lx - 0x%016lx] usable\r\n", entry->base,
               entry->base + entry->size);
    }
    printk("Memory size: %dk\r\n", memory_size / 1024);

    return 0;
}

void init_memory(void* dtb)
{
    of_scan_fdt(fdt_scan_root, NULL, dtb);
    of_scan_fdt(fdt_scan_memory, NULL, dtb);

    slabs_init();
}

void copy_from_user(void* dst, const void* src, size_t len)
{
    /* TODO: check if user address is valid */
    enable_user_access();
    memcpy(dst, src, len);
    disable_user_access();
}
