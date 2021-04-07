#include "virtio.h"
#include "const.h"
#include "errno.h"
#include "fdt.h"
#include "global.h"
#include "proto.h"

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

struct virtio_queue* vring_create_virtqueue(struct virtio_dev* dev,
                                            unsigned int index,
                                            unsigned int num,
                                            int (*notify)(struct virtio_queue*))
{
    if (num & (num - 1)) {
        printk("queue number is not power of 2\n");
        return NULL;
    }

    struct virtio_queue* q;
    SLABALLOC(q);
    if (!q) {
        return NULL;
    }

    q->dev = dev;
    q->index = index;
    q->num = num;
    q->notify = notify;

    q->size = virtq_size(q->num, PG_SIZE);
    q->phys_addr = alloc_pages(q->size >> PG_SHIFT);
    if (q->phys_addr == 0) {
        SLABFREE(q);
        return NULL;
    }
    q->vir_addr = __va(q->phys_addr);

    init_phys_queue(q);

    return q;
}

int virtio_write_queue(struct virtio_queue* q, int qidx,
                       struct virtio_buffer* bufs, int num)
{
    int i;
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

int virtio_kick_queue(struct virtio_queue* vq)
{
    mb();

    if (!(vq->virtq.used->flags & 1))
        return vq->notify(vq);
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
