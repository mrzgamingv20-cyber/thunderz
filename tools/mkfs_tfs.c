#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TFS_MAGIC 0x54484653
#define TFS_VERSION 1
#define TFS_SECTOR_SIZE 512
#define TFS_FILENAME_MAX 32
#define TFS_MAX_INODES 64
#define TFS_DIRECT_PTRS 10
#define TFS_INODE_SIZE 128

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

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: mkfs.tfs <disk.img> [size_mb]\n");
        return 1;
    }

    int size_mb = argc > 2 ? atoi(argv[2]) : 4;
    uint64_t total_sectors = (uint64_t)size_mb * 1024 * 1024 / TFS_SECTOR_SIZE;

    uint32_t inode_table_sectors = (TFS_MAX_INODES * TFS_INODE_SIZE + TFS_SECTOR_SIZE - 1) / TFS_SECTOR_SIZE;
    uint32_t data_sector = 1 + inode_table_sectors;

    struct tfs_super sb = {
        .magic = TFS_MAGIC,
        .version = TFS_VERSION,
        .total_sectors = total_sectors,
        .inode_count = TFS_MAX_INODES,
        .inode_table_sector = 1,
        .data_sector = data_sector,
    };

    FILE *f = fopen(argv[1], "wb");
    if (!f) { perror("fopen"); return 1; }

    uint8_t sector[TFS_SECTOR_SIZE];
    memset(sector, 0, TFS_SECTOR_SIZE);
    memcpy(sector, &sb, sizeof(sb));
    fwrite(sector, 1, TFS_SECTOR_SIZE, f);

    struct tfs_inode inodes[TFS_MAX_INODES];
    memset(inodes, 0, sizeof(inodes));
    fwrite(inodes, 1, sizeof(inodes), f);

    for (uint64_t i = data_sector; i < total_sectors; i++) {
        memset(sector, 0, TFS_SECTOR_SIZE);
        fwrite(sector, 1, TFS_SECTOR_SIZE, f);
    }

    fclose(f);
    printf("mkfs.tfs: created %s (%d MB, %lu sectors, data starts at %u)\n",
           argv[1], size_mb, (unsigned long)total_sectors, data_sector);
    return 0;
}