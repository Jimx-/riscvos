#include "global.h"
#include "proto.h"
#include "vm.h"

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    void* dtb = __va(dtb_phys);

    init_memory(dtb);
    init_trap();
    init_proc();

    switch_to_user();

    /* unreachable */
    return;
}
