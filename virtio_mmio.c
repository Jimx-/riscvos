#include "const.h"
#include "errno.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"
#include "virtio.h"

#define VIRTIO_DEV_MAX 16
static struct virtio_dev devices[VIRTIO_DEV_MAX];
static int num_devices = 0;

static uint32_t vm_read32(struct virtio_dev* dev, unsigned int off)
{
    return *(uint32_t*)(dev->vir_base + off);
}

/* static void vm_write8(struct virtio_dev* dev, unsigned int off, uint8_t val)
 */
/* { */
/*     *(uint8_t*)(dev->vir_base + off) = val; */
/* } */

/* static void vm_write16(struct virtio_dev* dev, unsigned int off, uint16_t
 * val) */
/* { */
/*     *(uint16_t*)(dev->vir_base + off) = val; */
/* } */

static void vm_write32(struct virtio_dev* dev, unsigned int off, uint32_t val)
{
    *(uint32_t*)(dev->vir_base + off) = val;
}

static void vm_get(struct virtio_dev* dev, unsigned offset, void* buf,
                   unsigned len)
{
    offset += VIRTIO_CONFIG_OFF;

    switch (len) {
    case 1:
        *(uint8_t*)buf = *(volatile uint8_t*)(dev->vir_base + offset);
        break;
    case 2:
        *(uint16_t*)buf = *(volatile uint16_t*)(dev->vir_base + offset);
        break;
    case 4:
        *(uint32_t*)buf = *(volatile uint32_t*)(dev->vir_base + offset);
        break;
    }
}

static void vm_set(struct virtio_dev* dev, unsigned offset, const void* buf,
                   unsigned len)
{
    offset += VIRTIO_CONFIG_OFF;

    switch (len) {
    case 1:
        *(volatile uint8_t*)(dev->vir_base + offset) = *(uint8_t*)buf;
        break;
    case 2:
        *(volatile uint16_t*)(dev->vir_base + offset) = *(uint16_t*)buf;
        break;
    case 4:
        *(volatile uint32_t*)(dev->vir_base + offset) = *(uint32_t*)buf;
        break;
    }
}

static uint8_t vm_get_status(struct virtio_dev* dev)
{
    uint32_t v;
    v = vm_read32(dev, VIRTIO_STATUS_OFF);
    return v & 0xff;
}

static void vm_set_status(struct virtio_dev* dev, uint8_t status)
{
    vm_write32(dev, VIRTIO_STATUS_OFF, status);
}

static int vm_get_features(struct virtio_dev* dev)
{
    uint32_t features;
    struct virtio_feature* f;
    int i;

    vm_write32(dev, VIRTIO_DEV_F_SEL_OFF, 0);
    features = vm_read32(dev, VIRTIO_DEV_F_OFF);

    for (i = 0; i < dev->num_features; i++) {
        f = &dev->features[i];

        f->dev_support = ((features >> f->bit) & 1);
    }

    return 0;
}

static int vm_finalize_features(struct virtio_dev* dev)
{
    uint32_t features;
    struct virtio_feature* f;
    int i;

    features = 0;
    for (i = 0; i < dev->num_features; i++) {
        f = &dev->features[i];

        features |= (f->drv_support << f->bit);
    }

    vm_write32(dev, VIRTIO_DRV_F_SEL_OFF, 0);
    vm_write32(dev, VIRTIO_DRV_F_OFF, features);

    return 0;
}

static int vm_notify(struct virtio_queue* vq)
{
    struct virtio_dev* dev = vq->dev;
    vm_write32(dev, VIRTIO_Q_NOTIFY_OFF, vq->index);
    return 1;
}

static int setup_vq(struct virtio_dev* dev, unsigned int qidx,
                    vq_callback_t callback, struct virtio_queue** qp)
{
    unsigned int num;
    struct virtio_queue* q;

    vm_write32(dev, VIRTIO_DRV_PG_SIZE_OFF, PG_SIZE);
    vm_write32(dev, VIRTIO_Q_SEL_OFF, qidx);

    num = vm_read32(dev, VIRTIO_Q_NUM_MAX_OFF);

    q = vring_create_virtqueue(dev, qidx, num, vm_notify, callback);

    vm_write32(dev, VIRTIO_Q_NUM_OFF, q->num);
    vm_write32(dev, VIRTIO_Q_ALIGN_OFF, PG_SIZE);

    vm_write32(dev, VIRTIO_Q_PFN_OFF, q->phys_addr >> PG_SHIFT);

    *qp = q;

    return 0;
}

static int vm_interrupt(int irq, void* opaque)
{
    struct virtio_dev* dev = opaque;
    int ret = 0;

    uint8_t status = vm_read32(dev, VIRTIO_INT_STATUS_OFF);
    vm_write32(dev, VIRTIO_INT_ACK_OFF, 1);

    if (status & 1) {
        /* vring interrupt */
        struct virtio_queue* vq;
        list_for_each_entry(vq, &dev->virtqueues, list)
        {
            ret |= virtqueue_interrupt(irq, vq);
        }
    }

    return ret;
}

static int vm_find_vqs(struct virtio_dev* dev, unsigned int nvqs,
                       struct virtio_queue* vqs[], vq_callback_t callbacks[])
{
    unsigned int i;

    if (nvqs < 0 || nvqs >= 256) {
        return EINVAL;
    }

    put_irq_handler(dev->irq, vm_interrupt, dev);

    for (i = 0; i < nvqs; i++) {
        setup_vq(dev, i, callbacks[i], &vqs[i]);
    }

    return 0;
}

static int vm_had_irq(struct virtio_dev* dev)
{
    uint8_t status = vm_read32(dev, VIRTIO_INT_STATUS_OFF);
    return status & 1;
}

static void vm_reset(struct virtio_dev* dev) { vm_set_status(dev, 0); }

static const struct virtio_config_ops virtio_mmio_config_ops = {
    .get = vm_get,
    .set = vm_set,
    .get_status = vm_get_status,
    .set_status = vm_set_status,
    .get_features = vm_get_features,
    .finalize_features = vm_finalize_features,
    .reset = vm_reset,
    .find_vqs = vm_find_vqs,
    .had_irq = vm_had_irq,
};

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

    dev->version = vm_read32(dev, VIRTIO_VERSION_OFF);
    dev->did = vm_read32(dev, VIRTIO_DEVICE_ID_OFF);
    if (dev->did == 0) return 0;

    dev->vid = vm_read32(dev, VIRTIO_VENDOR_ID_OFF);

    dev->config = &virtio_mmio_config_ops;
    num_devices++;
    return num_devices >= VIRTIO_DEV_MAX;
}

void init_virtio_mmio(void* dtb) { of_scan_fdt(fdt_scan_virtio, NULL, dtb); }

struct virtio_dev* virtio_mmio_get_dev(unsigned device_id)
{
    struct virtio_dev* dev;
    for (dev = devices; dev < devices + num_devices; dev++) {
        if (dev->did == device_id) return dev;
    }

    return NULL;
}
