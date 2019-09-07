#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <stddef.h>
#include <stdint.h>

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
    void* vir_addr;
    unsigned long phys_addr;

    uint16_t num;
    uint32_t size;
    struct virtq virtq;
};

struct virtio_buffer {
    unsigned long phys_addr;
    size_t size;
    int write;
};

struct virtio_dev {
    unsigned long reg_base;
    size_t reg_size;
    void* vir_base;

    unsigned int version;
    unsigned int vid, did;

    struct virtio_feature* features;
    int num_features;

    struct virtio_queue* queues;
    int num_queues;
    size_t queue_size;

    int irq;
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

void init_virtio();

struct virtio_dev* virtio_get_dev(unsigned int did,
                                  struct virtio_feature* features,
                                  int num_features);

int virtio_alloc_queues(struct virtio_dev* dev, int num_queues);
int virtio_write_queue(struct virtio_dev* dev, int qidx,
                       struct virtio_buffer* bufs, int num);

int virtio_device_supports(struct virtio_dev* dev, int bit);

void virtio_device_ready(struct virtio_dev* dev);

int virtio_had_irq(struct virtio_dev* dev);
void virtio_ack_irq(struct virtio_dev* dev);

uint32_t virtio_read32(struct virtio_dev* dev, unsigned int off);
void virtio_write8(struct virtio_dev* dev, unsigned int off, uint8_t val);
void virtio_write16(struct virtio_dev* dev, unsigned int off, uint16_t val);
void virtio_write32(struct virtio_dev* dev, unsigned int off, uint32_t val);

uint8_t virtio_cread8(struct virtio_dev* dev, unsigned int off);
uint16_t virtio_cread16(struct virtio_dev* dev, unsigned int off);
uint32_t virtio_cread32(struct virtio_dev* dev, unsigned int off);

#endif
