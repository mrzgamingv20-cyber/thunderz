#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TFS_MAGIC 0x54484653
#define TFS_VERSION 1
#define TFS_SECTOR_SIZE 512
#define TFS_FILENAME_MAX 32
#define TFS_MAX_INODES 64
#define TFS_DIRECT_PTRS 20
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
};
_Static_assert(sizeof(struct tfs_inode) == TFS_INODE_SIZE, "inode size");

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: mkfs.tfs <disk.img> [size_mb] [file=path ...]\n");
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

    struct tfs_inode inodes[TFS_MAX_INODES];
    memset(inodes, 0, sizeof(inodes));

    uint8_t *data = calloc(total_sectors - data_sector, TFS_SECTOR_SIZE);
    if (!data) { fprintf(stderr, "out of memory\n"); return 1; }

    int ino_idx = 0;
    for (int a = 3; a < argc; a++) {
        char *eq = strchr(argv[a], '=');
        if (!eq) { fprintf(stderr, "bad arg %s (want name=path)\n", argv[a]); return 1; }
        *eq = 0;
        const char *name = argv[a];
        const char *path = eq + 1;

        FILE *f = fopen(path, "rb");
        if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        uint32_t nblocks = (sz + TFS_SECTOR_SIZE - 1) / TFS_SECTOR_SIZE;
        if (nblocks > TFS_DIRECT_PTRS) {
            fprintf(stderr, "%s too big (%ld > %d)\n", path, sz, TFS_DIRECT_PTRS * TFS_SECTOR_SIZE);
            return 1;
        }
        if (ino_idx >= TFS_MAX_INODES) { fprintf(stderr, "too many files\n"); return 1; }

        struct tfs_inode *ino = &inodes[ino_idx];
        ino->flags = TFS_INODE_USED;
        snprintf(ino->filename, TFS_FILENAME_MAX, "%s", name);
        ino->size = sz;

        uint8_t buf[TFS_SECTOR_SIZE];
        for (uint32_t b = 0; b < nblocks; b++) {
            ino->blocks[b] = data_sector + ino_idx * TFS_DIRECT_PTRS + b;
            size_t off = (size_t)(ino_idx * TFS_DIRECT_PTRS + b) * TFS_SECTOR_SIZE;
            memset(buf, 0, sizeof(buf));
            size_t remain = sz - (size_t)b * TFS_SECTOR_SIZE;
            if (remain > TFS_SECTOR_SIZE) remain = TFS_SECTOR_SIZE;
            if (fread(buf, 1, remain, f) != remain) { fprintf(stderr, "read error %s\n", path); return 1; }
            memcpy(data + off, buf, remain);
        }
        fclose(f);
        ino_idx++;
    }

    FILE *f = fopen(argv[1], "wb");
    if (!f) { perror("fopen"); return 1; }

    uint8_t sector[TFS_SECTOR_SIZE];
    memset(sector, 0, TFS_SECTOR_SIZE);
    memcpy(sector, &sb, sizeof(sb));
    fwrite(sector, 1, TFS_SECTOR_SIZE, f);
    fwrite(inodes, 1, sizeof(inodes), f);

    uint32_t inode_pad = inode_table_sectors * TFS_SECTOR_SIZE - sizeof(inodes);
    memset(sector, 0, TFS_SECTOR_SIZE);
    for (uint32_t i = 0; i < inode_pad / TFS_SECTOR_SIZE; i++)
        fwrite(sector, 1, TFS_SECTOR_SIZE, f);

    fwrite(data, 1, (total_sectors - data_sector) * TFS_SECTOR_SIZE, f);
    fclose(f);
    free(data);

    printf("mkfs.tfs: created %s (%d MB, %lu sectors, data starts at %u, %d files)\n",
           argv[1], size_mb, (unsigned long)total_sectors, data_sector, ino_idx);
    return 0;
}
