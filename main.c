#include "proto.h"
#include "global.h"
#include "vm.h"

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    void* dtb = __va(dtb_phys);
    init_memory(dtb);

    while (1)
        ;

    /* unreachable */
    return;
}
