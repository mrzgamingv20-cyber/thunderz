#ifndef VIRTIO_H
#define VIRTIO_H

#include "../include/types.h"

#define VIRTIO_MMIO_BASE 0x0A000000
#define VIRTIO_MMIO_STRIDE 0x200
#define VIRTIO_MMIO_COUNT 32

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_DEV_BLK 2

#define VIRTIO_REG_MAGIC 0x000
#define VIRTIO_REG_VERSION 0x004
#define VIRTIO_REG_DEVICE_ID 0x008
#define VIRTIO_REG_VENDOR_ID 0x00C
#define VIRTIO_REG_DEVICE_FEATURES 0x010
#define VIRTIO_REG_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_REG_DRIVER_FEATURES 0x020
#define VIRTIO_REG_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_REG_QUEUE_SEL 0x030
#define VIRTIO_REG_QUEUE_SIZE_MAX 0x034
#define VIRTIO_REG_QUEUE_SIZE 0x038
#define VIRTIO_REG_QUEUE_READY 0x044
#define VIRTIO_REG_QUEUE_NOTIFY 0x050
#define VIRTIO_REG_INTERRUPT_STATUS 0x060
#define VIRTIO_REG_INTERRUPT_ACK 0x064
#define VIRTIO_REG_STATUS 0x070
#define VIRTIO_REG_QUEUE_DESC_LOW 0x080
#define VIRTIO_REG_QUEUE_DESC_HIGH 0x084
#define VIRTIO_REG_QUEUE_DRIVER_LOW 0x090
#define VIRTIO_REG_QUEUE_DRIVER_HIGH 0x094
#define VIRTIO_REG_QUEUE_DEVICE_LOW 0x0A0
#define VIRTIO_REG_QUEUE_DEVICE_HIGH 0x0A4
#define VIRTIO_REG_CONFIG 0x100

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8

#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

#define VIRTIO_BLK_S_OK 0

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
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
    struct virtq_used_elem ring[];
} __attribute__((packed));

struct virtio_blk_outhdr {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
} __attribute__((packed));

#define virtio_wmb() asm volatile("dmb ishst" ::: "memory")
#define virtio_rmb() asm volatile("dmb ishld" ::: "memory")

uint32_t virtio_read32(uint64_t base, uint32_t off);
void virtio_write32(uint64_t base, uint32_t off, uint32_t val);
int virtio_probe_blk(uint64_t *base_out);

#endif