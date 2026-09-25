#include "../include/types.h"
#include "../include/string.h"
#include "virtio.h"
#include "blk.h"
#include "uart.h"

#define SECTOR_SIZE 512
#define QUEUE_SIZE 8

#define VIRTIO_LEGACY_GUEST_PAGE_SIZE 0x028
#define VIRTIO_LEGACY_QUEUE_ALIGN 0x03C
#define VIRTIO_LEGACY_QUEUE_PFN 0x040

static uint64_t blk_base;
static uint64_t blk_capacity_sectors;
static int legacy_mode;

static struct virtq_desc *vq_desc;
static struct virtq_avail *vq_avail;
static struct virtq_used *vq_used;
static uint16_t last_used;

static uint8_t status_byte __attribute__((aligned(512)));
static struct virtio_blk_outhdr outhdr __attribute__((aligned(512)));
static uint8_t sector_buf[SECTOR_SIZE] __attribute__((aligned(512)));

static uint8_t vq_region[8192] __attribute__((aligned(4096)));

static void vq_setup(void) {
    uint64_t desc_sz = sizeof(struct virtq_desc) * QUEUE_SIZE;
    uint64_t avail_sz = 6 + sizeof(uint16_t) * QUEUE_SIZE;
    uint64_t avail_end = desc_sz + avail_sz;

    vq_desc = (struct virtq_desc*)vq_region;
    vq_avail = (struct virtq_avail*)(vq_region + desc_sz);

    uint64_t used_off = (avail_end + 4095) & ~4095UL;
    vq_used = (struct virtq_used*)(vq_region + used_off);

    memset(vq_region, 0, sizeof(vq_region));
    last_used = 0;

    virtio_write32(blk_base, VIRTIO_REG_QUEUE_SEL, 0);
    uint32_t qmax = virtio_read32(blk_base, VIRTIO_REG_QUEUE_SIZE_MAX);
    if (qmax == 0) return;

    virtio_write32(blk_base, VIRTIO_REG_QUEUE_SIZE, QUEUE_SIZE);

    if (legacy_mode) {
        virtio_write32(blk_base, VIRTIO_LEGACY_GUEST_PAGE_SIZE, 4096);
        virtio_write32(blk_base, VIRTIO_LEGACY_QUEUE_ALIGN, 4096);
        virtio_write32(blk_base, VIRTIO_LEGACY_QUEUE_PFN, (uint32_t)((uint64_t)vq_region >> 12));
    } else {
        uint64_t desc_pa = (uint64_t)vq_desc;
        uint64_t avail_pa = (uint64_t)vq_avail;
        uint64_t used_pa = (uint64_t)vq_used;
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DESC_LOW, (uint32_t)desc_pa);
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DESC_HIGH, (uint32_t)(desc_pa >> 32));
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DRIVER_LOW, (uint32_t)avail_pa);
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DRIVER_HIGH, (uint32_t)(avail_pa >> 32));
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DEVICE_LOW, (uint32_t)used_pa);
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_DEVICE_HIGH, (uint32_t)(used_pa >> 32));
        virtio_write32(blk_base, VIRTIO_REG_QUEUE_READY, 1);
    }
}

int blk_init(void) {
    if (virtio_probe_blk(&blk_base) < 0) return -1;

    virtio_write32(blk_base, VIRTIO_REG_STATUS, 0);
    uint32_t ver = virtio_read32(blk_base, VIRTIO_REG_VERSION);
    legacy_mode = (ver == 1);

    virtio_write32(blk_base, VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE);
    virtio_write32(blk_base, VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    virtio_write32(blk_base, VIRTIO_REG_DEVICE_FEATURES_SEL, 0);
    virtio_read32(blk_base, VIRTIO_REG_DEVICE_FEATURES);
    virtio_write32(blk_base, VIRTIO_REG_DRIVER_FEATURES_SEL, 0);
    virtio_write32(blk_base, VIRTIO_REG_DRIVER_FEATURES, 0);

    if (legacy_mode) {
        virtio_write32(blk_base, VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);
    } else {
        virtio_write32(blk_base, VIRTIO_REG_STATUS, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);
        uint32_t st = virtio_read32(blk_base, VIRTIO_REG_STATUS);
        if (!(st & VIRTIO_STATUS_FEATURES_OK)) return -1;
    }

    vq_setup();

    volatile uint32_t *cap = (volatile uint32_t*)(blk_base + VIRTIO_REG_CONFIG);
    blk_capacity_sectors = (uint64_t)cap[0] | ((uint64_t)cap[1] << 32);

    uint32_t ok = VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_DRIVER_OK;
    if (!legacy_mode) ok |= VIRTIO_STATUS_FEATURES_OK;
    virtio_write32(blk_base, VIRTIO_REG_STATUS, ok);
    return 0;
}

uint64_t blk_capacity(void) {
    return blk_capacity_sectors;
}

static int blk_request(uint32_t type, uint64_t sector, void *buf) {
    status_byte = 0xFF;
    outhdr.type = type;
    outhdr.ioprio = 0;
    outhdr.sector = sector;

    uint16_t head = 0;
    vq_desc[0].addr = (uint64_t)&outhdr;
    vq_desc[0].len = sizeof(outhdr);
    vq_desc[0].flags = VIRTQ_DESC_F_NEXT;
    vq_desc[0].next = 1;

    vq_desc[1].addr = (uint64_t)buf;
    vq_desc[1].len = SECTOR_SIZE;
    vq_desc[1].flags = VIRTQ_DESC_F_NEXT | (type == VIRTIO_BLK_T_IN ? VIRTQ_DESC_F_WRITE : 0);
    vq_desc[1].next = 2;

    vq_desc[2].addr = (uint64_t)&status_byte;
    vq_desc[2].len = 1;
    vq_desc[2].flags = VIRTQ_DESC_F_WRITE;
    vq_desc[2].next = 0;

    vq_avail->ring[vq_avail->idx % QUEUE_SIZE] = head;
    virtio_wmb();
    vq_avail->idx++;
    virtio_wmb();

    virtio_write32(blk_base, VIRTIO_REG_QUEUE_NOTIFY, 0);

    uint32_t timeout = 5000000;
    while (vq_used->idx == last_used) {
        timeout--;
        if (timeout == 0) return -1;
        asm volatile("yield");
    }
    virtio_rmb();
    last_used++;

    return status_byte == VIRTIO_BLK_S_OK ? 0 : -1;
}

int blk_read(uint64_t sector, void *buf, uint32_t count) {
    uint8_t *dst = buf;
    for (uint32_t i = 0; i < count; i++) {
        if (blk_request(VIRTIO_BLK_T_IN, sector + i, sector_buf) < 0) return -1;
        memcpy(dst + i * SECTOR_SIZE, sector_buf, SECTOR_SIZE);
    }
    return 0;
}

int blk_write(uint64_t sector, const void *buf, uint32_t count) {
    const uint8_t *src = buf;
    for (uint32_t i = 0; i < count; i++) {
        memcpy(sector_buf, src + i * SECTOR_SIZE, SECTOR_SIZE);
        if (blk_request(VIRTIO_BLK_T_OUT, sector + i, sector_buf) < 0) return -1;
    }
    return 0;
}
