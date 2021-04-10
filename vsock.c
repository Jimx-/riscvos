#include "const.h"
#include "errno.h"
#include "global.h"
#include "proto.h"
#include "string.h"
#include "virtio.h"

#include "virtio_vsock.h"

static struct virtio_dev* vsock_dev;
static struct virtio_vsock_config vsock_config;

enum {
    VSOCK_VQ_RX = 0, /* for host to guest data */
    VSOCK_VQ_TX = 1, /* for guest to host data */
    VSOCK_VQ_EVENT = 2,
    VSOCK_VQ_MAX = 3,
};
static struct virtio_queue* vqs[VSOCK_VQ_MAX];
static int rx_buf_nr;
static int rx_max_buf_nr;

static uint32_t local_port = 8000;

struct virtio_vsock_pkt {
    struct virtio_vsock_hdr hdr;
    struct list_head list;
    void* buf;
    size_t buf_len;
    size_t len;
};

static void virtio_vsock_rx_done(struct virtio_queue* vq);
static void virtio_vsock_tx_done(struct virtio_queue* vq);
static void virtio_vsock_event_done(struct virtio_queue* vq);

static void virtio_vsock_free_pkt(struct virtio_vsock_pkt* pkt)
{
    if (pkt->buf) vmfree(pkt->buf, pkt->buf_len);
    SLABFREE(pkt);
}

static void virtio_vsock_fill_rx(void)
{
    size_t buf_len = PG_SIZE;
    struct virtio_queue* vq = vqs[VSOCK_VQ_RX];
    struct virtio_vsock_pkt* pkt;
    struct virtio_buffer bufs[2];
    int retval;

    while (vq->free_num) {
        SLABALLOC(pkt);
        if (!pkt) break;
        memset(pkt, 0, sizeof(*pkt));

        pkt->buf_len = roundup(buf_len, PG_SIZE);
        pkt->buf = vmalloc_pages(pkt->buf_len >> PG_SHIFT, NULL);
        if (!pkt->buf) {
            virtio_vsock_free_pkt(pkt);
            break;
        }

        pkt->len = buf_len;

        bufs[0].phys_addr = __pa(&pkt->hdr);
        bufs[0].size = sizeof(pkt->hdr);
        bufs[0].write = 1;

        bufs[1].phys_addr = __pa(pkt->buf);
        bufs[1].size = sizeof(buf_len);
        bufs[1].write = 1;

        retval = virtqueue_add_buffers(vq, bufs, 2, pkt);
        if (retval) {
            virtio_vsock_free_pkt(pkt);
            break;
        }
        rx_buf_nr++;
    }

    if (rx_buf_nr > rx_max_buf_nr) rx_max_buf_nr = rx_buf_nr;

    virtqueue_kick(vq);
}

int init_vsock(void)
{
    int retval;
    vq_callback_t callbacks[] = {
        virtio_vsock_rx_done,
        virtio_vsock_tx_done,
        virtio_vsock_event_done,
    };

    vsock_dev = virtio_probe_device(19, NULL, 0);

    if (!vsock_dev) {
        printk("vsock: no vsock device found\n\r");
        return ENXIO;
    }

    retval = virtio_find_vqs(vsock_dev, VSOCK_VQ_MAX, vqs, callbacks);
    if (retval) return retval;

    virtio_cread(vsock_dev, struct virtio_vsock_config, guest_cid,
                 &vsock_config.guest_cid);
    printk("vosck: guest_cid=%ld\r\n", vsock_config.guest_cid);

    virtio_device_ready(vsock_dev);

    virtio_vsock_fill_rx();

    return 0;
}

static void virtio_vsock_rx_done(struct virtio_queue* vq)
{
    void* data;
    size_t len;

    while (!virtqueue_get_buffer(vq, &len, &data)) {
        struct virtio_vsock_pkt* pkt = data;

        rx_buf_nr--;

        if (len < sizeof(pkt->hdr) || len > sizeof(pkt->hdr) + pkt->len) {
            virtio_vsock_free_pkt(pkt);
            continue;
        }

        pkt->len = len - sizeof(pkt->hdr);
        virtio_vsock_free_pkt(pkt);
    }

    if (rx_buf_nr < rx_max_buf_nr / 2) virtio_vsock_fill_rx();
}

static void virtio_vsock_tx_done(struct virtio_queue* vq)
{
    void* data;

    while (!virtqueue_get_buffer(vq, NULL, &data)) {
        struct virtio_vsock_pkt* pkt = data;
        virtio_vsock_free_pkt(pkt);
    }
}

static void virtio_vsock_event_done(struct virtio_queue* vq)
{
    printk("Event done\r\n");
}

static struct virtio_vsock_pkt*
virtio_vsock_alloc_pkt(uint16_t type, uint64_t op, size_t len, uint32_t src_cid,
                       uint32_t src_port, uint32_t dst_cid, uint32_t dst_port)
{
    struct virtio_vsock_pkt* pkt;

    SLABALLOC(pkt);
    if (!pkt) return NULL;
    memset(pkt, 0, sizeof(*pkt));

    pkt->hdr.type = type;
    pkt->hdr.op = op;
    pkt->hdr.src_cid = src_cid;
    pkt->hdr.src_port = src_port;
    pkt->hdr.dst_cid = dst_cid;
    pkt->hdr.dst_port = dst_port;
    pkt->hdr.len = len;
    pkt->len = len;

    if (len) {
        pkt->buf_len = roundup(len, PG_SIZE);
        pkt->buf = vmalloc_pages(pkt->buf_len >> PG_SHIFT, NULL);
        if (!pkt->buf) goto out_pkt;
    }

    return pkt;

out_pkt:
    SLABFREE(pkt);
    return NULL;
}

int virtio_vsock_connect(uint32_t dst_cid, uint32_t dst_port)
{
    uint32_t src_cid = vsock_config.guest_cid;
    uint32_t src_port = local_port;
    struct virtio_queue* vq = vqs[VSOCK_VQ_TX];
    struct virtio_buffer buf;
    struct virtio_vsock_pkt* pkt;
    int retval;

    pkt = virtio_vsock_alloc_pkt(VIRTIO_VSOCK_TYPE_STREAM,
                                 VIRTIO_VSOCK_OP_REQUEST, 0, src_cid, src_port,
                                 dst_cid, dst_port);
    if (!pkt) return ENOMEM;

    buf.phys_addr = __pa(&pkt->hdr);
    buf.size = sizeof(pkt->hdr);
    buf.write = 0;

    retval = virtqueue_add_buffers(vq, &buf, 1, pkt);
    if (retval) return -retval;

    virtqueue_kick(vq);

    return 0;
}
