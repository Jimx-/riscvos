#include "global.h"
#include "irq.h"
#include "pci.h"
#include "proto.h"
#include "virtio.h"
#include "vm.h"

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    void* dtb = __va(dtb_phys);

    init_memory(dtb);
    init_timer(dtb);
    init_virtio(dtb);
    init_pci_host(dtb);

    init_trap();
    /* init_proc(); */

    init_blkdev();

    uint8_t buf[1024];
    blk_rdwt(0, 0, 1, buf);

    /* local_irq_enable(); */

    /* restart_local_timer(); */

    while (1)
        ;

    /* switch_to_user(); */

    /* unreachable */
    return;
}
