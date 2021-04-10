#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <stddef.h>
#include <stdint.h>

#include "list.h"

#include "virtio_ring.h"

struct virtio_dev;
struct virtio_queue;

struct virtio_feature {
    uint8_t bit;
    uint8_t dev_support;
    uint8_t drv_support;
};

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __attribute__((packed));

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
} __attribute__((packed));

struct virtq {
    int num;
    struct virtq_desc* descs;
    struct virtq_avail* avail;
    struct virtq_used* used;
};

struct virtio_queue {
    struct list_head list;
    struct virtio_dev* dev;
    void* vir_addr;
    unsigned long phys_addr;

    int (*notify)(struct virtio_queue*);

    void* priv;

    unsigned int index;
    uint16_t num;
    uint32_t size;
    struct vring vring;

    unsigned int free_num;
    unsigned int free_head;
    unsigned int free_tail;
    unsigned int last_used;

    void** data;
    size_t data_size;

    void (*callback)(struct virtio_queue*);
};

typedef void (*vq_callback_t)(struct virtio_queue*);

struct virtio_buffer {
    unsigned long phys_addr;
    size_t size;
    int write;
};

struct virtio_config_ops {
    void (*get)(struct virtio_dev* vdev, unsigned offset, void* buf,
                unsigned len);
    void (*set)(struct virtio_dev* vdev, unsigned offset, const void* buf,
                unsigned len);
    uint8_t (*get_status)(struct virtio_dev* vdev);
    void (*set_status)(struct virtio_dev* vdev, uint8_t status);
    void (*reset)(struct virtio_dev* dev);
    int (*had_irq)(struct virtio_dev* vdev);
    int (*get_features)(struct virtio_dev* vdev);
    int (*finalize_features)(struct virtio_dev* vdev);
    int (*find_vqs)(struct virtio_dev* vdev, unsigned nvqs,
                    struct virtio_queue* vqs[], vq_callback_t callbacks[]);
    void (*del_vqs)(struct virtio_dev* vdev);
};

struct virtio_pci_common_cfg;

struct virtio_dev {
    unsigned long reg_base;
    size_t reg_size;
    void* vir_base;

    unsigned int version;
    unsigned int vid, did;

    uint8_t* isr;
    struct virtio_pci_common_cfg* common;
    void* device;
    void* notify_base;
    int modern_bars;

    size_t notify_len;
    size_t device_len;
    int notify_map_cap;
    uint32_t notify_offset_multiplier;

    struct virtio_feature* features;
    int num_features;

    struct list_head virtqueues;
    size_t queue_size;

    int irq;

    const struct virtio_config_ops* config;

    void* private;
};

#define VIRTIO_VERSION_OFF 0x004
#define VIRTIO_DEVICE_ID_OFF 0x008
#define VIRTIO_VENDOR_ID_OFF 0x00c
#define VIRTIO_DEV_F_OFF 0x010
#define VIRTIO_DEV_F_SEL_OFF 0x014
#define VIRTIO_DRV_F_OFF 0x020
#define VIRTIO_DRV_F_SEL_OFF 0x024
#define VIRTIO_DRV_PG_SIZE_OFF 0x028
#define VIRTIO_Q_SEL_OFF 0x030
#define VIRTIO_Q_NUM_MAX_OFF 0x034
#define VIRTIO_Q_NUM_OFF 0x038
#define VIRTIO_Q_ALIGN_OFF 0x03c
#define VIRTIO_Q_PFN_OFF 0x040
#define VIRTIO_Q_NOTIFY_OFF 0x050
#define VIRTIO_INT_STATUS_OFF 0x060
#define VIRTIO_INT_ACK_OFF 0x064
#define VIRTIO_STATUS_OFF 0x070
#define VIRTIO_CONFIG_OFF 0x100

/* Device status register */
#define VIRTIO_STATUS_ACK 0x1
#define VIRTIO_STATUS_DRV 0x2
#define VIRTIO_STATUS_DRV_OK 0x4

struct virtio_queue* vring_create_virtqueue(struct virtio_dev* dev,
                                            unsigned int index,
                                            unsigned int num,
                                            int (*notify)(struct virtio_queue*),
                                            vq_callback_t callback);

void init_virtio_mmio();

struct virtio_dev* virtio_mmio_get_dev(unsigned device_id);
struct virtio_dev* virtio_pci_get_dev(unsigned device_id);

struct virtio_dev* virtio_probe_device(uint32_t subdid,
                                       struct virtio_feature* features,
                                       int num_features);

int virtio_alloc_queues(struct virtio_dev* dev, unsigned int nvqs,
                        struct virtio_queue* vqs[]);

int virtqueue_add_buffers(struct virtio_queue* vq, struct virtio_buffer* bufs,
                          size_t count, void* data);
int virtqueue_kick(struct virtio_queue* vq);
int virtqueue_interrupt(int irq, struct virtio_queue* vq);

int virtio_device_supports(struct virtio_dev* dev, int bit);

static inline int virtio_find_vqs(struct virtio_dev* dev, unsigned nvqs,
                                  struct virtio_queue* vqs[],
                                  vq_callback_t callbacks[])
{
    return dev->config->find_vqs(dev, nvqs, vqs, callbacks);
}

static inline int virtio_had_irq(struct virtio_dev* dev)
{
    return dev->config->had_irq(dev);
}

static inline uint8_t virtio_cread8(struct virtio_dev* vdev, unsigned offset)
{
    uint8_t v;

    vdev->config->get(vdev, offset, &v, sizeof(v));
    return v;
}

static inline uint16_t virtio_cread16(struct virtio_dev* vdev, unsigned offset)
{
    uint16_t v;

    vdev->config->get(vdev, offset, &v, sizeof(v));
    return v;
}

static inline uint32_t virtio_cread32(struct virtio_dev* vdev, unsigned offset)
{
    uint32_t v;

    vdev->config->get(vdev, offset, &v, sizeof(v));
    return v;
}

static inline uint64_t virtio_cread64(struct virtio_dev* vdev, unsigned offset)
{
    uint64_t v;

    vdev->config->get(vdev, offset, &v, sizeof(v));
    return v;
}

static inline void virtio_cwrite32(struct virtio_dev* vdev, unsigned offset,
                                   uint32_t val)
{
    vdev->config->set(vdev, offset, &val, sizeof(val));
}

#define virtio_cread(vdev, structname, member, ptr)                      \
    do {                                                                 \
        switch (sizeof(*ptr)) {                                          \
        case 4:                                                          \
            *(ptr) = virtio_cread32(vdev, offsetof(structname, member)); \
            break;                                                       \
        case 8:                                                          \
            *(ptr) = virtio_cread64(vdev, offsetof(structname, member)); \
            break;                                                       \
        default:                                                         \
            break;                                                       \
        }                                                                \
    } while (0)

static inline void virtio_device_ready(struct virtio_dev* dev)
{
    unsigned int status = dev->config->get_status(dev);
    dev->config->set_status(dev, status | VIRTIO_STATUS_DRV_OK);
}

#endif
