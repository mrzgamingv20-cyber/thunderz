#ifndef TFS_H
#define TFS_H

#include "../include/types.h"

#define TFS_MAGIC 0x54484653
#define TFS_VERSION 1
#define TFS_SECTOR_SIZE 512
#define TFS_FILENAME_MAX 32
#define TFS_MAX_INODES 64
#define TFS_DIRECT_PTRS 10
#define TFS_INODE_SIZE 128

#define TFS_INODE_FREE 0
#define TFS_INODE_USED 1

struct tfs_super {
    uint32_t magic;
    uint32_t version;
    uint64_t total_sectors;
    uint32_t inode_count;
    uint32_t inode_table_sector;
    uint32_t data_sector;
    uint32_t _pad;
} __attribute__((packed));

struct tfs_inode {
    uint32_t flags;
    char filename[TFS_FILENAME_MAX];
    uint32_t _pad1;
    uint64_t size;
    uint32_t blocks[TFS_DIRECT_PTRS];
    uint32_t _pad2[10];
};
_Static_assert(sizeof(struct tfs_inode) == TFS_INODE_SIZE, "inode size");

int tfs_mount(void);
int tfs_read(const char *path, void *buf, uint64_t len);
int tfs_write(const char *path, const void *buf, uint64_t len);
int tfs_list(void (*callback)(const char *name, uint64_t size));
int tfs_unlink(const char *path);

#endif