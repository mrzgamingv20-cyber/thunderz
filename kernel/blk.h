#ifndef BLK_H
#define BLK_H

#include "../include/types.h"

int blk_init(void);
int blk_read(uint64_t sector, void *buf, uint32_t count);
int blk_write(uint64_t sector, const void *buf, uint32_t count);
uint64_t blk_capacity(void);

#endif