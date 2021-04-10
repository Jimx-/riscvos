#include "const.h"
#include "errno.h"
#include "global.h"
#include "proto.h"
#include "virtio.h"

#include <string.h>

static struct virtio_dev* blk_dev;
static struct virtio_queue* vq;

/* Feature bits */
#define VIRTIO_BLK_F_BARRIER 0   /* Does host support barriers? */
#define VIRTIO_BLK_F_SIZE_MAX 1  /* Indicates maximum segment size */
#define VIRTIO_BLK_F_SEG_MAX 2   /* Indicates maximum # of segments */
#define VIRTIO_BLK_F_GEOMETRY 4  /* Legacy geometry available  */
#define VIRTIO_BLK_F_RO 5        /* Disk is read-only */
#define VIRTIO_BLK_F_BLK_SIZE 6  /* Block size of disk is available*/
#define VIRTIO_BLK_F_SCSI 7      /* Supports scsi command passthru */
#define VIRTIO_BLK_F_FLUSH 9     /* Cache flush command support */
#define VIRTIO_BLK_F_TOPOLOGY 10 /* Topology information is available */

static struct virtio_feature blk_features[] = {
    {VIRTIO_BLK_F_BARRIER, 0, 0}, {VIRTIO_BLK_F_SIZE_MAX, 0, 0},
    {VIRTIO_BLK_F_SEG_MAX, 0, 0}, {VIRTIO_BLK_F_GEOMETRY, 0, 0},
    {VIRTIO_BLK_F_RO, 0, 0},      {VIRTIO_BLK_F_BLK_SIZE, 0, 0},
    {VIRTIO_BLK_F_SCSI, 0, 0},    {VIRTIO_BLK_F_TOPOLOGY, 0, 0},
};

struct virtio_blk_config {
    /* The capacity (in 512-byte sectors). */
    uint64_t capacity;
    /* The maximum segment size (if VIRTIO_BLK_F_SIZE_MAX) */
    uint32_t size_max;
    /* The maximum number of segments (if VIRTIO_BLK_F_SEG_MAX) */
    uint32_t seg_max;
    /* geometry the device (if VIRTIO_BLK_F_GEOMETRY) */
    struct virtio_blk_geometry {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;

    /* block size of device (if VIRTIO_BLK_F_BLK_SIZE) */
    uint32_t blk_size;

    /* the next 4 entries are guarded by VIRTIO_BLK_F_TOPOLOGY  */
    /* exponent for physical block per logical block. */
    uint8_t physical_block_exp;
    /* alignment offset in logical blocks. */
    uint8_t alignment_offset;
    /* minimum I/O size without performance penalty in logical blocks. */
    uint16_t min_io_size;
    /* optimal sustained I/O size in logical blocks. */
    uint32_t opt_io_size;
} __attribute__((packed)) blk_config;

#define VIRTIO_BLK_BLOCK_SIZE 512

struct virtio_blk_req_header {
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed)) req_hdr;

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2
uint8_t dev_status;

static int virtio_blk_config();
static void virtio_blk_done(struct virtio_queue* vq);

int init_blkdev()
{
    int retval;
    vq_callback_t callback = virtio_blk_done;

    blk_dev = virtio_probe_device(
        2, blk_features, sizeof(blk_features) / sizeof(struct virtio_feature));

    if (!blk_dev) {
        printk("blkdev: no block device found\n\r");
        return ENXIO;
    }

    retval = virtio_find_vqs(blk_dev, 1, &vq, &callback);
    if (retval) return retval;

    retval = virtio_blk_config();
    if (retval) return retval;

    virtio_device_ready(blk_dev);

    return 0;
}

static int virtio_blk_config()
{
    uint32_t cap_low, cap_high;
    cap_low = virtio_cread32(blk_dev, 0);
    cap_high = virtio_cread32(blk_dev, 4);
    blk_config.capacity = ((uint64_t)cap_high << 32) | cap_low;

    printk("blkdev: disk size %d MB\r\n",
           (blk_config.capacity * VIRTIO_BLK_BLOCK_SIZE) >> 20);

    if (virtio_device_supports(blk_dev, VIRTIO_BLK_F_SEG_MAX)) {
        blk_config.seg_max = virtio_cread32(blk_dev, 12);
        printk("blkdev: segment max=%d\r\n", blk_config.seg_max);
    }

    if (virtio_device_supports(blk_dev, VIRTIO_BLK_F_GEOMETRY)) {
        blk_config.geometry.cylinders = virtio_cread16(blk_dev, 16);
        blk_config.geometry.heads = virtio_cread8(blk_dev, 18);
        blk_config.geometry.sectors = virtio_cread8(blk_dev, 19);

        printk("blkdev: cylinders=%d heads=%d sectors=%d\r\n",
               blk_config.geometry.cylinders, blk_config.geometry.heads,
               blk_config.geometry.sectors);
    }

    if (virtio_device_supports(blk_dev, VIRTIO_BLK_F_BLK_SIZE)) {
        blk_config.blk_size = virtio_cread32(blk_dev, 20);
        printk("blkdev: block size=%d\r\n", blk_config.blk_size);
    }

    return 0;
}

static int virtio_blk_status_to_errno(int status)
{
    switch (status) {
    case VIRTIO_BLK_S_IOERR:
        return EIO;
    case VIRTIO_BLK_S_UNSUPP:
        return ENOTSUP;
    default:
        panic("unknown dev status");
    }
    return 0;
}

static int64_t virtio_blk_rdwt(int write, uint64_t position, size_t size,
                               uint8_t* buf)
{
    struct virtio_buffer bufs[3];
    int retval;

    if (position >= blk_config.capacity * VIRTIO_BLK_BLOCK_SIZE) {
        return -EINVAL;
    }

    if (position % VIRTIO_BLK_BLOCK_SIZE) {
        return -EINVAL;
    }

    if (size % VIRTIO_BLK_BLOCK_SIZE) {
        return -EINVAL;
    }

    memset(&req_hdr, 0, sizeof(req_hdr));

    if (write) {
        req_hdr.type = VIRTIO_BLK_T_OUT;
    } else {
        req_hdr.type = VIRTIO_BLK_T_IN;
    }
    req_hdr.reserved = 0;
    req_hdr.sector = position / VIRTIO_BLK_BLOCK_SIZE;

    /* chain the buffers to make the request */
    bufs[0].phys_addr = __pa(&req_hdr);
    bufs[0].size = sizeof(req_hdr);
    bufs[0].write = 0;
    bufs[1].phys_addr = __pa(buf);
    bufs[1].size = size;
    bufs[1].write = !write;
    bufs[2].phys_addr = __pa(&dev_status);
    bufs[2].size = 1;
    bufs[2].write = 1;

    retval = virtqueue_add_buffers(vq, bufs, 3, NULL);
    if (retval) return -retval;

    virtqueue_kick(vq);

    /* if (dev_status == 0) { */
    /*     return size; */
    /* } */

    /* return -virtio_blk_status_to_errno(dev_status); */
    return 0;
}

static void virtio_blk_done(struct virtio_queue* vq) { printk("blk done\r\n"); }

int blk_rdwt(int write, unsigned int block_num, size_t count, uint8_t* buf)
{
    int64_t retval =
        virtio_blk_rdwt(write, block_num * BLOCK_SIZE, count * BLOCK_SIZE, buf);

    if (retval < 0) return -retval;
    return 0;
}
