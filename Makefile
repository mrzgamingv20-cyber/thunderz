CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS = -ffreestanding -fno-builtin -mgeneral-regs-only -nostdlib -nostartfiles -Wall -Wextra -O2 -Iinclude
LDFLAGS = -T linker.ld

OBJS = boot.o kernel.o uart.o virtio.o blk.o tfs.o string.o

all: thunderz.bin disk.img

boot.o: boot/boot.S
	$(CC) -c -o $@ $<

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

virtio.o: kernel/virtio.c
	$(CC) $(CFLAGS) -c -o $@ $<

blk.o: kernel/blk.c
	$(CC) $(CFLAGS) -c -o $@ $<

tfs.o: kernel/tfs.c
	$(CC) $(CFLAGS) -c -o $@ $<

string.o: lib/string.c
	$(CC) $(CFLAGS) -c -o $@ $<

thunderz.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

thunderz.bin: thunderz.elf
	$(OBJCOPY) -O binary $< $@

tools/mkfs.tfs: tools/mkfs_tfs.c
	gcc -O2 -o $@ $<

disk.img: tools/mkfs.tfs
	./tools/mkfs.tfs disk.img 4

clean:
	rm -f *.o thunderz.elf thunderz.bin disk.img tools/mkfs.tfs

.PHONY: all clean