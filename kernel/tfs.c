#include "../include/types.h"
#include "../include/string.h"
#include "blk.h"
#include "tfs.h"

static struct tfs_super sb;
static int mounted;

static struct tfs_inode inodes[TFS_MAX_INODES];
static int dirty;

static void load_inodes(void) {
    uint8_t buf[TFS_SECTOR_SIZE];
    uint32_t per_sector = TFS_SECTOR_SIZE / TFS_INODE_SIZE;
    uint32_t total_sectors = (TFS_MAX_INODES + per_sector - 1) / per_sector;
    uint8_t *dst = (uint8_t*)inodes;

    for (uint32_t s = 0; s < total_sectors; s++) {
        if (blk_read(sb.inode_table_sector + s, buf, 1) < 0) break;
        memcpy(dst + s * TFS_SECTOR_SIZE, buf, TFS_SECTOR_SIZE);
    }
    dirty = 0;
}

static void flush_inodes(void) {
    if (!dirty) return;
    uint32_t per_sector = TFS_SECTOR_SIZE / TFS_INODE_SIZE;
    uint32_t total_sectors = (TFS_MAX_INODES + per_sector - 1) / per_sector;
    uint8_t *src = (uint8_t*)inodes;

    for (uint32_t s = 0; s < total_sectors; s++) {
        if (blk_write(sb.inode_table_sector + s, src + s * TFS_SECTOR_SIZE, 1) < 0) break;
    }
    dirty = 0;
}

int tfs_mount(void) {
    uint8_t buf[TFS_SECTOR_SIZE];
    if (blk_read(0, buf, 1) < 0) return -1;
    memcpy(&sb, buf, sizeof(sb));
    if (sb.magic != TFS_MAGIC) return -1;
    load_inodes();
    mounted = 1;
    return 0;
}

static struct tfs_inode *find_inode(const char *path) {
    for (int i = 0; i < TFS_MAX_INODES; i++) {
        if (inodes[i].flags == TFS_INODE_USED && strcmp(inodes[i].filename, path) == 0)
            return &inodes[i];
    }
    return NULL;
}

static struct tfs_inode *alloc_inode(const char *path) {
    for (int i = 0; i < TFS_MAX_INODES; i++) {
        if (inodes[i].flags == TFS_INODE_FREE) {
            memset(&inodes[i], 0, sizeof(struct tfs_inode));
            inodes[i].flags = TFS_INODE_USED;
            strcpy(inodes[i].filename, path);
            dirty = 1;
            return &inodes[i];
        }
    }
    return NULL;
}

int tfs_read(const char *path, void *buf, uint64_t len) {
    if (!mounted) return -1;
    struct tfs_inode *ino = find_inode(path);
    if (!ino) return -1;

    uint64_t to_read = len < ino->size ? len : ino->size;
    uint64_t done = 0;
    uint8_t *dst = buf;

    while (done < to_read) {
        uint32_t block_idx = done / TFS_SECTOR_SIZE;
        uint32_t offset = done % TFS_SECTOR_SIZE;
        if (block_idx >= TFS_DIRECT_PTRS || ino->blocks[block_idx] == 0) break;

        uint8_t sector_buf[TFS_SECTOR_SIZE];
        if (blk_read(ino->blocks[block_idx], sector_buf, 1) < 0) return -1;

        uint32_t chunk = TFS_SECTOR_SIZE - offset;
        if (chunk > to_read - done) chunk = to_read - done;
        memcpy(dst + done, sector_buf + offset, chunk);
        done += chunk;
    }
    return (int)done;
}

int tfs_write(const char *path, const void *buf, uint64_t len) {
    if (!mounted) return -1;
    struct tfs_inode *ino = find_inode(path);
    if (!ino) ino = alloc_inode(path);
    if (!ino) return -1;

    uint64_t done = 0;
    const uint8_t *src = buf;
    uint32_t first_free = 0;

    for (int i = 0; i < TFS_DIRECT_PTRS; i++) {
        if (ino->blocks[i] == 0) { first_free = i; break; }
        first_free = i + 1;
    }
    if (first_free >= TFS_DIRECT_PTRS) return -1;

    while (done < len) {
        uint32_t block_idx = done / TFS_SECTOR_SIZE;
        uint32_t offset = done % TFS_SECTOR_SIZE;
        if (block_idx >= TFS_DIRECT_PTRS) break;

        uint8_t sector_buf[TFS_SECTOR_SIZE];
        if (ino->blocks[block_idx] == 0) {
            uint32_t ino_idx = (uint32_t)(ino - inodes);
            ino->blocks[block_idx] = sb.data_sector + ino_idx * TFS_DIRECT_PTRS + block_idx;
            memset(sector_buf, 0, TFS_SECTOR_SIZE);
        } else {
            if (blk_read(ino->blocks[block_idx], sector_buf, 1) < 0) return -1;
        }

        uint32_t chunk = TFS_SECTOR_SIZE - offset;
        if (chunk > len - done) chunk = len - done;
        memcpy(sector_buf + offset, src + done, chunk);
        if (blk_write(ino->blocks[block_idx], sector_buf, 1) < 0) return -1;
        done += chunk;
    }

    ino->size = done;
    dirty = 1;
    flush_inodes();
    return (int)done;
}

int tfs_list(void (*callback)(const char *name, uint64_t size)) {
    if (!mounted) return -1;
    for (int i = 0; i < TFS_MAX_INODES; i++) {
        if (inodes[i].flags == TFS_INODE_USED)
            callback(inodes[i].filename, inodes[i].size);
    }
    return 0;
}

int tfs_unlink(const char *path) {
    if (!mounted) return -1;
    struct tfs_inode *ino = find_inode(path);
    if (!ino) return -1;
    memset(ino, 0, sizeof(struct tfs_inode));
    dirty = 1;
    flush_inodes();
    return 0;
}