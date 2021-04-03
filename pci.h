#ifndef _PCI_H_
#define _PCI_H_

struct pcibus {
    int busnr;

    unsigned long reg_base;
    size_t reg_size;
    void* win;

    int nr_resources;
    struct {
        int flags;
#define PCI_RESOURCE_IO 1
#define PCI_RESOURCE_MEM 2
        unsigned long cpu_addr;
        unsigned long pci_addr;
        size_t size;
        unsigned long alloc_offset;
    } resources[10];
};

struct pcidev {
    struct pcibus* bus;

    uint8_t busnr;
    uint8_t dev;
    uint8_t func;
    unsigned int devfn;

    uint16_t vid;
    uint16_t did;

    uint8_t baseclass;
    uint8_t subclass;
    uint8_t infclass;

    uint8_t headt;

    int nr_bars;
    struct {
        int nr;

        unsigned long base;
        size_t size;

        int flags;
    } bars[6];
};

#define PCI_VID          0x00   /* Vendor ID, 16-bit */
#define PCI_DID          0x02   /* Device ID, 16-bit */
#define PCI_CR           0x04   /* Command Register, 16-bit */
#define PCI_CR_MAST_EN   0x0004 /* Enable Busmaster Access */
#define PCI_CR_MEM_EN    0x0002 /* Enable Mem Cycles */
#define PCI_CR_IO_EN     0x0001 /* Enable I/O Cycles */
#define PCI_SR           0x06   /* PCI status, 16-bit */
#define PSR_SSE          0x4000 /* Signaled System Error */
#define PSR_RMAS         0x2000 /* Received Master Abort Status */
#define PSR_RTAS         0x1000 /* Received Target Abort Status */
#define PSR_CAPPTR       0x0010 /* Capabilities list */
#define PCI_REV          0x08   /* Revision ID */
#define PCI_PIFR         0x09   /* Prog. Interface Register */
#define PCI_SCR          0x0A   /* Sub-Class Register */
#define PCI_BCR          0x0B   /* Base-Class Register */
#define PCI_CLS          0x0C   /* Cache Line Size */
#define PCI_LT           0x0D   /* Latency Timer */
#define PCI_HEADT        0x0E   /* Header type, 8-bit */
#define PHT_MASK         0x7F   /* Header type mask */
#define PHT_NORMAL       0x00
#define PHT_BRIDGE       0x01
#define PHT_CARDBUS      0x02
#define PHT_MULTIFUNC    0x80       /* Multiple functions */
#define PCI_BIST         0x0F       /* Built-in Self Test */
#define PCI_BAR          0x10       /* Base Address Register */
#define PCI_BAR_IO       0x00000001 /* Reg. refers to I/O space */
#define PCI_BAR_TYPE     0x00000006 /* Memory BAR type */
#define PCI_TYPE_32      0x00000000 /* 32-bit BAR */
#define PCI_TYPE_32_1M   0x00000002 /* 32-bit below 1MB (legacy) */
#define PCI_TYPE_64      0x00000004 /* 64-bit BAR */
#define PCI_BAR_PREFETCH 0x00000008 /* Memory is prefetchable */
#define PCI_BAR_IO_MASK  0xFFFFFFFC /* I/O address mask */
#define PCI_BAR_MEM_MASK 0xFFFFFFF0 /* Memory address mask */
#define PCI_BAR_2        0x14       /* Base Address Register */
#define PCI_BAR_3        0x18       /* Base Address Register */
#define PCI_BAR_4        0x1C       /* Base Address Register */
#define PCI_BAR_5        0x20       /* Base Address Register */
#define PCI_BAR_6        0x24       /* Base Address Register */
#define PCI_CBCISPTR     0x28       /* Cardbus CIS Pointer */
#define PCI_SUBVID       0x2C       /* Subsystem Vendor ID */
#define PCI_SUBDID       0x2E       /* Subsystem Device ID */
#define PCI_EXPROM       0x30       /* Expansion ROM Base Address */
#define PCI_CAPPTR       0x34       /* Capabilities Pointer */
#define PCI_CP_MASK      0xfc       /* Lower 2 bits should be ignored */
#define PCI_ILR          0x3C       /* Interrupt Line Register */
#define PCI_ILR_UNKNOWN  0xFF       /* IRQ is unassigned or unknown */
#define PCI_IPR          0x3D       /* Interrupt Pin Register */
#define PCI_MINGNT       0x3E       /* Min Grant */
#define PCI_MAXLAT       0x3F       /* Max Latency */

void init_pci_host(void* dtb);

int pci_bus_read_config_byte(struct pcibus* bus, unsigned int devfn, int pos,
                             uint8_t* value);
int pci_bus_read_config_word(struct pcibus* bus, unsigned int devfn, int pos,
                             uint16_t* value);
int pci_bus_read_config_dword(struct pcibus* bus, unsigned int devfn, int pos,
                              uint32_t* value);
int pci_bus_write_config_byte(struct pcibus* bus, unsigned int devfn, int pos,
                             uint8_t value);
int pci_bus_write_config_word(struct pcibus* bus, unsigned int devfn, int pos,
                             uint16_t value);
int pci_bus_write_config_dword(struct pcibus* bus, unsigned int devfn, int pos,
                             uint32_t value);

#endif
