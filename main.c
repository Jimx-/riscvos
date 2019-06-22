#include "proto.h"

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    printk("kernel main\n");

    while (1)
        ;

    /* unreachable */
    return;
}
