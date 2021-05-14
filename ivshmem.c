#include "pci.h"
#include "proto.h"

#include <errno.h>

#define IVSHMEM_VENDOR_ID 0x1af4
#define IVSHMEM_DEVICE_ID 0x1110

static void* shmem_base;

int init_ivshmem(void)
{
    unsigned long base;
    size_t size;
    int iof, retval;

    struct pcidev* pdev = pci_get_device(IVSHMEM_VENDOR_ID, IVSHMEM_DEVICE_ID);
    if (!pdev) {
        printk("Inter-VM shared memory device not found.\r\n");
        return ENXIO;
    }

    retval = pci_get_bar(pdev, PCI_BAR + 8, &base, &size, &iof);
    shmem_base = vm_mapio(base, size);

    printk("ivshmem: base %p, size %d MB\r\n", shmem_base, size >> 20);

    return 0;
}
