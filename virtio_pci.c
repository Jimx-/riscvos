#include "const.h"
#include "errno.h"
#include "global.h"
#include "pci.h"
#include "proto.h"
#include "virtio.h"

#include "virtio_pci.h"

#define VIRTIO_VENDOR_ID 0x1AF4

static void* map_capability(struct pcidev* pdev, int off, size_t minlen,
                            uint32_t align, uint32_t start, uint32_t size,
                            size_t* len);

#define VP_READ_IO(bits)                                      \
    static uint##bits##_t vp_read##bits(uint##bits##_t* addr) \
    {                                                         \
        return *(volatile uint##bits##_t*)addr;               \
    }

VP_READ_IO(32)
VP_READ_IO(16)
VP_READ_IO(8)

#define VP_WRITE_IO(bits)                                                \
    static void vp_write##bits(uint##bits##_t* addr, uint##bits##_t val) \
    {                                                                    \
        *(volatile uint##bits##_t*)addr = val;                           \
    }

VP_WRITE_IO(32)
VP_WRITE_IO(16)
VP_WRITE_IO(8)

static void vp_write64(uint32_t* lo, uint32_t* hi, uint64_t val)
{
    vp_write32(lo, (uint32_t)val);
    vp_write32(hi, val >> 32);
}

static void vp_get(struct virtio_dev* vdev, unsigned offset, void* buf,
                   unsigned len)
{
    uint8_t b;
    uint16_t w;
    uint32_t l;

    switch (len) {
    case 1:
        b = vp_read8(vdev->device + offset);
        *(uint8_t*)buf = b;
        break;
    case 2:
        w = vp_read16(vdev->device + offset);
        *(uint16_t*)buf = w;
        break;
    case 4:
        l = vp_read32(vdev->device + offset);
        *(uint32_t*)buf = l;
        break;
    case 8:
        l = vp_read32(vdev->device + offset);
        *(uint32_t*)buf = l;
        l = vp_read32(vdev->device + offset + sizeof(l));
        *((uint32_t*)buf + 1) = l;
    }
}

static void vp_set(struct virtio_dev* vdev, unsigned offset, const void* buf,
                   unsigned len)
{
    uint8_t b;
    uint16_t w;
    uint32_t l;

    switch (len) {
    case 1:
        b = *(uint8_t*)buf;
        vp_write8(vdev->device + offset, b);
        break;
    case 2:
        w = *(uint16_t*)buf;
        vp_write16(vdev->device + offset, w);
        break;
    case 4:
        l = *(uint32_t*)buf;
        vp_write32(vdev->device + offset, l);
        break;
    case 8:
        l = *(uint32_t*)buf;
        vp_write32(vdev->device + offset, l);
        l = *((uint32_t*)buf + 1);
        vp_write32(vdev->device + offset + sizeof(l), l);
    }
}

static uint8_t vp_get_status(struct virtio_dev* vdev)
{
    return vp_read8(&vdev->common->device_status);
}

static void vp_set_status(struct virtio_dev* vdev, uint8_t status)
{
    vp_write8(&vdev->common->device_status, status);
}

static void vp_reset(struct virtio_dev* vdev)
{
    /* 0 = reset */
    vp_set_status(vdev, 0);
    /* flush out the status write */
    while (vp_get_status(vdev))
        ;
}

static int vp_get_features(struct virtio_dev* vdev)
{
    uint64_t features;
    struct virtio_feature* f;
    int i;

    vp_write32(&vdev->common->device_feature_select, 0);
    features = vp_read32(&vdev->common->device_feature);
    vp_write32(&vdev->common->device_feature_select, 1);
    features |= ((uint64_t)vp_read32(&vdev->common->device_feature) << 32);

    for (i = 0; i < vdev->num_features; i++) {
        f = &vdev->features[i];

        f->dev_support = ((features >> f->bit) & 1);
    }

    return 0;
}

static int vp_finalize_features(struct virtio_dev* vdev)
{
    uint64_t features;
    struct virtio_feature* f;
    int i;

    features = 0;
    for (i = 0; i < vdev->num_features; i++) {
        f = &vdev->features[i];

        features |= (f->drv_support << f->bit);
    }

    vp_write32(&vdev->common->guest_feature_select, 0);
    vp_write32(&vdev->common->guest_feature, (uint32_t)features);
    vp_write32(&vdev->common->guest_feature_select, 1);
    vp_write32(&vdev->common->guest_feature, features >> 32);

    return 0;
}

static int vp_had_irq(struct virtio_dev* vdev)
{
    return vp_read8(vdev->isr) & 1;
}

static int vp_notify(struct virtio_queue* vq)
{
    vp_write16(vq->priv, vq->index);
    return 1;
}

static int setup_vq(struct virtio_dev* vdev, unsigned index,
                    struct virtio_queue** vqp)
{
    struct virtio_queue* vq;
    struct virtio_pci_common_cfg* cfg = vdev->common;
    uint16_t num, off;
    unsigned long desc_addr, avail_addr, used_addr;
    int retval;

    if (index >= vp_read16(&cfg->num_queues)) {
        return ENOENT;
    }

    vp_write16(&cfg->queue_select, index);

    num = vp_read16(&cfg->queue_size);
    if (!num || vp_read16(&cfg->queue_enable)) {
        return ENOENT;
    }

    if (num & (num - 1)) {
        return EINVAL;
    }

    off = vp_read16(&cfg->queue_notify_off);

    vq = vring_create_virtqueue(vdev, index, num, vp_notify);
    if (!vq) {
        return ENOMEM;
    }

    vp_write16(&cfg->queue_size, vq->num);

    desc_addr = vq->phys_addr;
    avail_addr =
        vq->phys_addr + ((char*)vq->vring.avail - (char*)vq->vring.desc);
    used_addr =
        vq->phys_addr + ((char*)vq->vring.used - (char*)vq->vring.desc);

    vp_write64(&cfg->queue_desc_lo, &cfg->queue_desc_hi, desc_addr);
    vp_write64(&cfg->queue_avail_lo, &cfg->queue_avail_hi, avail_addr);
    vp_write64(&cfg->queue_used_lo, &cfg->queue_used_hi, used_addr);

    if (vdev->notify_base) {
        if ((uint64_t)off * vdev->notify_offset_multiplier + 2 >
            vdev->notify_len) {
            retval = EINVAL;
            goto err_del_queue;
        }
        vq->priv = vdev->notify_base + off * vdev->notify_offset_multiplier;
    } else {
        vq->priv =
            map_capability(vdev->private, vdev->notify_map_cap, 2, 2,
                           off * vdev->notify_offset_multiplier, 2, NULL);
    }

    if (!vq->priv) {
        retval = ENOMEM;
        goto err_del_queue;
    }

    *vqp = vq;
    return 0;

err_del_queue:
    /* vring_del_virtqueue(vq); */
    return retval;
}

static int vp_modern_find_vqs(struct virtio_dev* vdev, unsigned nvqs,
                              struct virtio_queue* vqs[])
{
    unsigned int i;
    int retval;

    if (nvqs < 0 || nvqs >= 256) {
        return EINVAL;
    }

    irq_unmask(vdev->irq);

    for (i = 0; i < nvqs; i++) {
        retval = setup_vq(vdev, i, &vqs[i]);
        if (retval) return retval;
    }

    for (i = 0; i < nvqs; i++) {
        vp_write16(&vdev->common->queue_select, vqs[i]->index);
        vp_write16(&vdev->common->queue_enable, 1);
    }

    return 0;
}

static const struct virtio_config_ops virtio_pci_config_nodev_ops = {
    .get = NULL,
    .set = NULL,
    .get_status = vp_get_status,
    .set_status = vp_set_status,
    .reset = vp_reset,
    .had_irq = vp_had_irq,
    .get_features = vp_get_features,
    .finalize_features = vp_finalize_features,
    .find_vqs = vp_modern_find_vqs,
};

static const struct virtio_config_ops virtio_pci_config_ops = {
    .get = vp_get,
    .set = vp_set,
    .get_status = vp_get_status,
    .set_status = vp_set_status,
    .reset = vp_reset,
    .had_irq = vp_had_irq,
    .get_features = vp_get_features,
    .finalize_features = vp_finalize_features,
    .find_vqs = vp_modern_find_vqs,
};

static inline int virtio_pci_find_capability(struct pcidev* dev,
                                             uint8_t cfg_type, int ioflag,
                                             int* bars)
{
    int pos;
    uint8_t type, bar;
    unsigned long base;
    size_t size;
    int iof, retval;

    for (pos = pci_find_capability(dev, PCI_CAP_ID_VNDR); pos > 0;
         pos = pci_find_next_capability(dev, pos, PCI_CAP_ID_VNDR)) {

        pci_read_attr_byte(dev, pos + VIRTIO_PCI_CAP_CFG_TYPE, &type);
        pci_read_attr_byte(dev, pos + VIRTIO_PCI_CAP_BAR, &bar);

        if (bar > 0x5) continue;

        if (type == cfg_type) {
            retval = pci_get_bar(dev, PCI_BAR + bar * 4, &base, &size, &iof);

            if (retval == 0 && size > 0 && iof == ioflag) {
                *bars |= 1 << bar;
                return pos;
            }
        }
    }

    return 0;
}

static void* map_capability(struct pcidev* pdev, int off, size_t minlen,
                            uint32_t align, uint32_t start, uint32_t size,
                            size_t* len)
{
    uint8_t bar;
    uint32_t offset, length;
    unsigned long bar_base;
    size_t bar_size;
    int ioflag, retval;
    void* p;

    pci_read_attr_byte(pdev, off + VIRTIO_PCI_CAP_BAR, &bar);
    pci_read_attr_dword(pdev, off + VIRTIO_PCI_CAP_OFFSET, &offset);
    pci_read_attr_dword(pdev, off + VIRTIO_PCI_CAP_LENGTH, &length);

    retval =
        pci_get_bar(pdev, PCI_BAR + bar * 4, &bar_base, &bar_size, &ioflag);
    if (retval) return NULL;
    if (ioflag) return NULL;

    if (length <= start || length - start < minlen) {
        return NULL;
    }

    length -= start;

    if (start + offset < offset) {
        return NULL;
    }

    offset += start;

    if (offset & (align - 1)) {
        return NULL;
    }

    if (length > size) length = size;

    if (len) *len = length;

    if (minlen + offset < minlen || minlen + offset > bar_size) {
        return NULL;
    }

    p = vm_mapio(bar_base + offset, length);

    return p;
}

struct virtio_dev* virtio_pci_get_dev(unsigned device_id)
{
    int common, isr, notify, device;
    uint32_t notify_offset, notify_length;

    struct pcidev* pdev = pci_get_device(VIRTIO_VENDOR_ID, 0x1040 + device_id);
    if (!pdev) return NULL;

    struct virtio_dev* vdev;
    SLABALLOC(vdev);
    if (!vdev) return NULL;

    vdev->private = pdev;

    common = virtio_pci_find_capability(pdev, VIRTIO_PCI_CAP_COMMON_CFG, 0,
                                        &vdev->modern_bars);
    if (!common) goto err_free_dev;

    isr = virtio_pci_find_capability(pdev, VIRTIO_PCI_CAP_ISR_CFG, 0,
                                     &vdev->modern_bars);
    notify = virtio_pci_find_capability(pdev, VIRTIO_PCI_CAP_NOTIFY_CFG, 0,
                                        &vdev->modern_bars);
    if (!isr || !notify) goto err_free_dev;

    device = virtio_pci_find_capability(pdev, VIRTIO_PCI_CAP_DEVICE_CFG, 0,
                                        &vdev->modern_bars);

    vdev->common =
        map_capability(pdev, common, sizeof(struct virtio_pci_common_cfg), 4, 0,
                       sizeof(struct virtio_pci_common_cfg), NULL);
    if (!vdev->common) goto err_free_dev;

    vdev->isr = map_capability(pdev, isr, sizeof(uint8_t), 1, 0, 1, NULL);
    if (!vdev->isr) goto err_free_dev;

    pci_read_attr_dword(pdev, notify + VIRTIO_PCI_NOTIFY_CAP_MULT,
                        &vdev->notify_offset_multiplier);
    pci_read_attr_dword(pdev, notify + VIRTIO_PCI_CAP_OFFSET, &notify_offset);
    pci_read_attr_dword(pdev, notify + VIRTIO_PCI_CAP_LENGTH, &notify_length);

    if ((uint64_t)notify_length + (notify_offset % PG_SIZE) <= PG_SIZE) {
        vdev->notify_base = map_capability(pdev, notify, 2, 2, 0, notify_length,
                                           &vdev->notify_len);
        if (!vdev->notify_base) goto err_free_dev;
    } else {
        vdev->notify_map_cap = notify;
    }

    if (device) {
        vdev->device =
            map_capability(pdev, device, 0, 4, 0, PG_SIZE, &vdev->device_len);
        if (!vdev->device) {
            goto err_free_dev;
        }

        vdev->config = &virtio_pci_config_ops;
    } else {
        vdev->config = &virtio_pci_config_nodev_ops;
    }

    vdev->irq = pci_get_intx_irq(pdev);

    return vdev;

err_free_dev:
    SLABFREE(vdev);
    return NULL;
}
