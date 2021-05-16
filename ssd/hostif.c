#include "hostif.h"
#include "byteorder.h"
#include "const.h"
#include "proto.h"
#include "ringbuf.h"

#include <assert.h>

/* Message types. */
#define MT_READ 1
#define MT_WRITE 2
#define MT_READ_COMP 3

#define MESSAGE_RINGBUF_DEFAULT_CAPACITY (PG_SIZE * 4 - 1)

static ringbuf_t pcie_message_ringbuf;

static void consume_pcie_messsage(const char* buf, size_t len)
{
    uint16_t type;
    uint64_t addr;
    uint32_t id;

    assert(len >= 10);
    type = be16_to_cpup((uint16_t*)buf);
    addr = ((unsigned long)be32_to_cpup((uint32_t*)&buf[2]) << 32) |
           be32_to_cpup((uint32_t*)&buf[6]);

    switch (type) {
    case MT_READ:
        id = *(uint32_t*)&buf[10];
        nvme_process_read_message(addr, id);
        break;
    case MT_WRITE:
        nvme_process_write_message(addr, &buf[10], len - 10);
        break;
    default:
        break;
    }
}

static void process_message_ringbuf(void)
{
    uint16_t msg_len;
    char msg_buf[512];

    for (;;) {
        /* No message. */
        if (ringbuf_bytes_used(pcie_message_ringbuf) < sizeof(uint16_t)) return;

        ringbuf_memcpy_from(&msg_len, pcie_message_ringbuf, sizeof(msg_len));
        msg_len = be16_to_cpup(&msg_len);

        assert(ringbuf_bytes_used(pcie_message_ringbuf) >= msg_len);
        assert(msg_len < sizeof(msg_buf));

        ringbuf_memcpy_from(msg_buf, pcie_message_ringbuf, msg_len);

        consume_pcie_messsage(msg_buf, msg_len);
    }
}

static void hostif_process_pcie_message(uint32_t src_cid, uint32_t src_port,
                                        const char* buf, size_t len)
{
    ringbuf_memcpy_into(pcie_message_ringbuf, buf, len);

    process_message_ringbuf();
}

int hostif_complete_host_read(uint32_t id, const char* buf, size_t len)
{
    char msg[1024];
    size_t msg_len = 2 + 4 + len;

    assert(msg_len + 2 <= sizeof(msg));

    *(uint16_t*)&msg[0] = __builtin_bswap16(msg_len);
    *(uint16_t*)&msg[2] = __builtin_bswap16((uint16_t)MT_READ_COMP);
    *(uint32_t*)&msg[4] = id;
    memcpy(&msg[8], buf, len);

    virtio_vsock_send(VSOCK_HOST_CID, VSOCK_HOST_PORT, msg, 2 + msg_len);
}

void hostif_init(void)
{
    pcie_message_ringbuf = ringbuf_new(MESSAGE_RINGBUF_DEFAULT_CAPACITY);

    virtio_vsock_set_recv_callback(hostif_process_pcie_message);
}
