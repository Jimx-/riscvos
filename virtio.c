#include "virtio.h"
#include "const.h"
#include "errno.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"

struct virtio_queue* vring_create_virtqueue(struct virtio_dev* vdev,
                                            unsigned int index,
                                            unsigned int num,
                                            int (*notify)(struct virtio_queue*))
{
    struct virtio_queue* vq;
    int i;

    if (num & (num - 1)) {
        printk("%s: bad queue length: %d\r\n", num);
        return NULL;
    }

    SLABALLOC(vq);
    if (!vq) return NULL;

    memset(vq, 0, sizeof(*vq));
    vq->index = index;
    vq->num = num;
    vq->size = vring_size(vq->num, PG_SIZE);

    vq->phys_addr = alloc_pages(vq->size >> PG_SHIFT);
    if (vq->phys_addr == 0) goto out_free_vq;
    vq->vir_addr = __va(vq->phys_addr);

    memset(vq->vir_addr, 0, vq->size);
    vring_init(&vq->vring, vq->num, vq->vir_addr, PG_SIZE);

    vq->dev = vdev;
    vq->notify = notify;

    vq->vring.used->flags = 0;
    vq->vring.avail->flags = 0;
    vq->vring.used->idx = 0;
    vq->vring.avail->idx = 0;

    for (i = 0; i < vq->num; i++) {
        vq->vring.desc[i].flags = VRING_DESC_F_NEXT;
        vq->vring.desc[i].next = (i + 1) & (vq->num - 1);

        vq->vring.avail->ring[i] = 0xffff;
        vq->vring.used->ring[i].id = 0xffff;
    }

    vq->free_num = vq->num;
    vq->free_head = 0;
    vq->free_tail = vq->num - 1;
    vq->last_used = 0;

    return vq;

out_free_vq:
    SLABFREE(vq);
    return NULL;
}

static inline void use_vring_desc(struct vring_desc* vd,
                                  struct virtio_buffer* buf)
{
    vd->addr = buf->phys_addr;
    vd->len = buf->size;
    vd->flags = VRING_DESC_F_NEXT;

    if (buf->write) vd->flags |= VRING_DESC_F_WRITE;
}

static void set_direct_descriptors(struct virtio_queue* vq,
                                   struct virtio_buffer* bufs, size_t count)
{
    u16 i;
    size_t j;
    struct vring* vring = &vq->vring;
    struct vring_desc* vd;

    if (0 == count) return;

    for (i = vq->free_head, j = 0; j < count; j++) {
        vd = &vring->desc[i];

        use_vring_desc(vd, &bufs[j]);
        i = vd->next;
    }

    vd->flags = vd->flags & ~VRING_DESC_F_NEXT;

    vq->free_num -= count;
    vq->free_head = i;
}

int virtqueue_add_buffers(struct virtio_queue* vq, struct virtio_buffer* bufs,
                          size_t count, void* data)
{
    struct vring* vring = &vq->vring;
    u16 old_head;

    old_head = vq->free_head;

    set_direct_descriptors(vq, bufs, count);

    vring->avail->ring[vring->avail->idx % vq->num] = old_head;

    mb();
    vring->avail->idx++;
    mb();

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

static int virtqueue_notify(struct virtio_queue* vq) { return vq->notify(vq); }

int virtqueue_kick(struct virtio_queue* vq)
{
    mb();

    if (!(vq->vring.used->flags & VRING_USED_F_NO_NOTIFY))
        return virtqueue_notify(vq);
    else
        return 0;
}

void virtio_add_status(struct virtio_dev* vdev, unsigned int status)
{
    vdev->config->set_status(vdev, vdev->config->get_status(vdev) | status);
}

static int negotiate_features(struct virtio_dev* dev)
{
    int retval;
    retval = dev->config->get_features(dev);
    if (retval) return retval;

    return dev->config->finalize_features(dev);
}

struct virtio_dev* virtio_probe_device(uint32_t subdid,
                                       struct virtio_feature* features,
                                       int num_features)
{
    int retval;
    struct virtio_dev* dev = virtio_mmio_get_dev(subdid);
    if (!dev) {
        dev = virtio_pci_get_dev(subdid);
    }
    if (!dev) return NULL;

    dev->features = features;
    dev->num_features = num_features;

    dev->config->reset(dev);
    virtio_add_status(dev, VIRTIO_STATUS_ACK);

    retval = negotiate_features(dev);
    if (retval) return NULL;

    virtio_add_status(dev, VIRTIO_STATUS_DRV);

    return dev;
}
