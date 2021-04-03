#include "virtio.h"
#include "const.h"
#include "errno.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"

#define VIRTIO_DEV_MAX 16
static struct virtio_dev devices[VIRTIO_DEV_MAX];
static int num_devices = 0;

static void negotiate_features(struct virtio_dev* dev);

static inline void virtq_init(struct virtq* vq, unsigned int num, void* p,
                              unsigned long align)
{
    vq->num = num;
    vq->descs = p;
    vq->avail = (void*)((char*)p + num * sizeof(struct virtq_desc));
    vq->used = (void*)(((unsigned long)&vq->avail->ring[num] +
                        sizeof(uint16_t) + align - 1) &
                       ~(align - 1));
}

static inline unsigned virtq_size(unsigned int num, unsigned long align)
{
    return ((sizeof(struct virtq_desc) * num + sizeof(uint16_t) * (3 + num) +
             align - 1) &
            ~(align - 1)) +
           sizeof(uint16_t) * 3 + sizeof(struct virtq_desc) * num;
}

static int fdt_scan_virtio(void* blob, unsigned long offset, const char* name,
                           int depth, void* arg)
{
    const char* type = fdt_getprop(blob, offset, "compatible", NULL);
    if (!type || strcmp(type, "virtio,mmio") != 0) return 0;

    struct virtio_dev* dev = &devices[num_devices];
    const uint32_t *reg, *irq;
    int len;
    reg = fdt_getprop(blob, offset, "reg", &len);
    if (!reg) return 0;

    uint64_t base, size;

    base = of_read_number(reg, dt_root_addr_cells);
    reg += dt_root_addr_cells;
    size = of_read_number(reg, dt_root_size_cells);
    reg += dt_root_size_cells;

    irq = fdt_getprop(blob, offset, "interrupts", NULL);
    if (!irq) return 0;
    dev->irq = be32_to_cpup(irq);

    dev->reg_base = base;
    dev->reg_size = size;

    dev->vir_base = vm_mapio(dev->reg_base, dev->reg_size);

    dev->version = virtio_read32(dev, VIRTIO_VERSION_OFF);
    dev->did = virtio_read32(dev, VIRTIO_DEVICE_ID_OFF);
    if (dev->did == 0) return 0;

    dev->vid = virtio_read32(dev, VIRTIO_VENDOR_ID_OFF);

    num_devices++;
    return num_devices >= VIRTIO_DEV_MAX;
}

void init_virtio(void* dtb) { of_scan_fdt(fdt_scan_virtio, NULL, dtb); }

struct virtio_dev* virtio_get_dev(unsigned device_id,
                                  struct virtio_feature* features,
                                  int num_features)
{
    struct virtio_dev* dev;
    for (dev = devices; dev < devices + num_devices; dev++) {
        if (dev->did != device_id) continue;

        virtio_write8(dev, VIRTIO_STATUS_OFF, 0);
        virtio_write8(dev, VIRTIO_STATUS_OFF, VIRTIO_STATUS_ACK);

        dev->features = features;
        dev->num_features = num_features;
        negotiate_features(dev);

        virtio_write8(dev, VIRTIO_STATUS_OFF, VIRTIO_STATUS_DRV);

        return dev;
    }

    return NULL;
}

static void negotiate_features(struct virtio_dev* dev)
{
    uint32_t dev_features, drv_features = 0;

    virtio_write32(dev, VIRTIO_DEV_F_SEL_OFF, 0);
    dev_features = virtio_read32(dev, VIRTIO_DEV_F_OFF);

    struct virtio_feature* f;
    for (f = dev->features; f < dev->features + dev->num_features; f++) {
        f->dev_support = (dev_features >> f->bit) & 1;
        drv_features |= (f->drv_support << f->bit);
    }

    virtio_write32(dev, VIRTIO_DRV_F_SEL_OFF, 0);
    virtio_write32(dev, VIRTIO_DRV_F_OFF, drv_features);
}

static int init_phys_queue(struct virtio_queue* q)
{
    memset(q->vir_addr, 0, q->size);

    virtq_init(&q->virtq, q->num, q->vir_addr, PG_SIZE);

    int i;
    for (i = 0; i < q->num; i++) {
        q->virtq.descs[i].flags = VIRTQ_DESC_F_NEXT;
        q->virtq.descs[i].next = (i + 1) % q->num;
    }

    return 0;
}

static int init_phys_queues(struct virtio_dev* dev)
{
    virtio_write32(dev, VIRTIO_DRV_PG_SIZE_OFF, PG_SIZE);

    struct virtio_queue* q;
    int i = 0;
    for (q = dev->queues; q < dev->queues + dev->num_queues; q++, i++) {
        virtio_write32(dev, VIRTIO_Q_SEL_OFF, i);

        q->num = virtio_read32(dev, VIRTIO_Q_NUM_MAX_OFF);

        if (q->num & (q->num - 1)) {
            printk("queue number is not power of 2\n");
            return EINVAL;
        }

        virtio_write32(dev, VIRTIO_Q_NUM_OFF, q->num);
        virtio_write32(dev, VIRTIO_Q_ALIGN_OFF, PG_SIZE);

        q->size = virtq_size(q->num, PG_SIZE);
        q->phys_addr = alloc_pages(q->size >> PG_SHIFT);
        if (q->phys_addr == 0) {
            return ENOMEM;
        }
        q->vir_addr = __va(q->phys_addr);

        init_phys_queue(q);

        virtio_write32(dev, VIRTIO_Q_PFN_OFF, q->phys_addr >> PG_SHIFT);
    }

    return 0;
}

int virtio_alloc_queues(struct virtio_dev* dev, int num_queues)
{
    if (num_queues < 0 || num_queues >= 256) {
        return EINVAL;
    }

    dev->queue_size =
        roundup(num_queues * sizeof(struct virtio_queue), PG_SIZE);
    dev->queues = __va(alloc_pages(dev->queue_size >> PG_SHIFT));
    memset(dev->queues, 0, dev->queue_size);
    dev->num_queues = num_queues;

    irq_unmask(dev->irq);

    int retval;
    if ((retval = init_phys_queues(dev)) != 0) {
        irq_mask(dev->irq);
        free_mem(__pa(dev->queues), dev->queue_size);
        dev->queues = NULL;
        return retval;
    }

    return 0;
}

static void notify_queue(struct virtio_dev* dev, int qidx)
{
    virtio_write32(dev, VIRTIO_Q_NOTIFY_OFF, qidx);
}

int virtio_write_queue(struct virtio_dev* dev, int qidx,
                       struct virtio_buffer* bufs, int num)
{
    int i;
    struct virtio_queue* q = &dev->queues[qidx];
    struct virtq* vq = &q->virtq;
    struct virtq_desc* vd;

    /* place buffers in the descriptor table */
    for (i = 0; i < num; i++) {
        vd = &vq->descs[i];
        struct virtio_buffer* vb = &bufs[i];
        vd->addr = vb->phys_addr;
        vd->len = vb->size;
        vd->flags = VIRTQ_DESC_F_NEXT;

        if (vb->write) {
            vd->flags |= VIRTQ_DESC_F_WRITE;
        }

        vd->next = (i + 1) % q->num;
    }
    vd->flags &= ~VIRTQ_DESC_F_NEXT;

    vq->avail->ring[vq->avail->idx % q->num] = 0;

    mb();
    vq->avail->idx++;
    mb();

    notify_queue(dev, qidx);

    return 0;
}

int virtio_device_supports(struct virtio_dev* dev, int bit)
{
    struct virtio_feature* f;
    for (f = dev->features; f < dev->features + dev->num_features; f++) {
        if (f->bit == bit) {
            return f->dev_support;
        }
    }

    return 0;
}

int virtio_had_irq(struct virtio_dev* dev)
{
    uint8_t status = virtio_read32(dev, VIRTIO_INT_STATUS_OFF);
    return status & 1;
}

void virtio_ack_irq(struct virtio_dev* dev)
{
    virtio_write32(dev, VIRTIO_INT_ACK_OFF, 1);
}

void virtio_device_ready(struct virtio_dev* dev)
{
    virtio_write8(dev, VIRTIO_STATUS_OFF, VIRTIO_STATUS_DRV_OK);
}

uint32_t virtio_read32(struct virtio_dev* dev, unsigned int off)
{
    return *(uint32_t*)(dev->vir_base + off);
}

void virtio_write8(struct virtio_dev* dev, unsigned int off, uint8_t val)
{
    *(uint8_t*)(dev->vir_base + off) = val;
}

void virtio_write16(struct virtio_dev* dev, unsigned int off, uint16_t val)
{
    *(uint16_t*)(dev->vir_base + off) = val;
}

void virtio_write32(struct virtio_dev* dev, unsigned int off, uint32_t val)
{
    *(uint32_t*)(dev->vir_base + off) = val;
}

uint8_t virtio_cread8(struct virtio_dev* dev, unsigned int off)
{
    return *(uint8_t*)(dev->vir_base + VIRTIO_CONFIG_OFF + off);
}

uint16_t virtio_cread16(struct virtio_dev* dev, unsigned int off)
{
    return *(uint16_t*)(dev->vir_base + VIRTIO_CONFIG_OFF + off);
}

uint32_t virtio_cread32(struct virtio_dev* dev, unsigned int off)
{
    return *(uint32_t*)(dev->vir_base + VIRTIO_CONFIG_OFF + off);
}
