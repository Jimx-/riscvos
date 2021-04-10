#include "global.h"
#include "irq.h"
#include "pci.h"
#include "proto.h"
#include "virtio.h"
#include "vm.h"

#include "smp.h"

#include "stackframe.h"

uint8_t buf[4096];

void kernel_main(unsigned int hart_id, void* dtb_phys)
{
    void* dtb = __va(dtb_phys);

    init_memory(dtb);
    init_timer(dtb);
    init_plic(dtb);
    init_virtio_mmio(dtb);
    init_pci_host(dtb);

    init_trap();
    /* init_proc(); */

    init_irq_cpu(smp_processor_id());
    local_irq_enable();

    init_vsock();

    init_blkdev();

    blk_rdwt(0, 0, 1, buf);
    /* blk_rdwt(0, 0, 1, buf); */
    /* blk_rdwt(0, 0, 1, buf); */

    printk("SMP %d\r\n", smp_processor_id());

    /* printk("%d\n", sizeof(struct reg_context)); */

    restart_local_timer();

    while (1)
        ;

    /* switch_to_user(); */

    /* unreachable */
    return;
}
