#include "proto.h"

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    direct_put_str("kernel main");

    while (1)
        ;

    /* unreachable */
    return;
}
