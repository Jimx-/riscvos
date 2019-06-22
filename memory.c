#include "fdt.h"
#include "global.h"

#include <stdint.h>

int dt_root_addr_cells, dt_root_size_cells;

int fdt_scan_root(void* blob, unsigned long offset, const char* name, int depth,
                  void* arg)
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
    const char* type = fdt_getprop(blob, offset, "device_type", NULL);
    if (!type || strcmp(type, "memory") != 0) return 0;

    const uint32_t *reg, *lim;
    int len;
    reg = fdt_getprop(blob, offset, "reg", &len);
    if (!reg) return 0;

    lim = reg + (len / sizeof(uint32_t));

    unsigned long memory_size = 0;
    printk("Physical RAM map:\n");
    while ((int)(lim - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
        uint64_t base, size;

        base = of_read_number(reg, dt_root_addr_cells);
        reg += dt_root_addr_cells;
        size = of_read_number(reg, dt_root_size_cells);
        reg += dt_root_size_cells;

        if (size == 0) continue;
        memory_size += size;

        printk("  mem[0x%016lx - 0x%016lx] usable\n", base, base + size);
    }
    printk("Memory size: %dk\n", memory_size / 1024);

    return 0;
}

void init_memory(void* dtb)
{
    of_scan_fdt(fdt_scan_root, NULL, dtb);
    of_scan_fdt(fdt_scan_memory, NULL, dtb);
}
