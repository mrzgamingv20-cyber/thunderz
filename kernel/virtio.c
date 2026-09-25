#include "../include/types.h"
#include "virtio.h"

static inline uint32_t mmio_r32(uint64_t addr) {
    return *(volatile uint32_t*)addr;
}

static inline void mmio_w32(uint64_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

uint32_t virtio_read32(uint64_t base, uint32_t off) {
    return mmio_r32(base + off);
}

void virtio_write32(uint64_t base, uint32_t off, uint32_t val) {
    mmio_w32(base + off, val);
}

int virtio_probe_blk(uint64_t *base_out) {
    for (int i = 0; i < VIRTIO_MMIO_COUNT; i++) {
        uint64_t base = VIRTIO_MMIO_BASE + (uint64_t)i * VIRTIO_MMIO_STRIDE;
        uint32_t magic = mmio_r32(base + VIRTIO_REG_MAGIC);
        if (magic != VIRTIO_MAGIC) continue;
        uint32_t devid = mmio_r32(base + VIRTIO_REG_DEVICE_ID);
        if (devid == VIRTIO_DEV_BLK) {
            *base_out = base;
            return 0;
        }
    }
    return -1;
}