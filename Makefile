CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS = -ffreestanding -fno-builtin -mgeneral-regs-only -nostdlib -nostartfiles -Wall -Wextra -O2 -Iinclude

BOOT_OBJS = boot1.o menu.o uart.o virtio.o blk.o tfs.o string.o
KERN_OBJS = boot2.o kernel.o uart.o virtio.o blk.o tfs.o string.o

all: bootloader.elf kernel.bin disk.img

boot1.o: boot/boot.S
	$(CC) $(CFLAGS) -DENTRY=boot_main -c -o $@ $<

boot2.o: boot/boot.S
	$(CC) $(CFLAGS) -DENTRY=kernel_main -c -o $@ $<

menu.o: boot/menu.c
	$(CC) $(CFLAGS) -c -o $@ $<

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

bootloader.elf: $(BOOT_OBJS) linker-boot.ld
	$(LD) -T linker-boot.ld -o $@ $(BOOT_OBJS)

kernel.elf: $(KERN_OBJS) linker-kernel.ld
	$(LD) -T linker-kernel.ld -o $@ $(KERN_OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

tools/mkfs.tfs: tools/mkfs_tfs.c
	gcc -O2 -o $@ $<

disk.img: tools/mkfs.tfs kernel.bin
	./tools/mkfs.tfs disk.img 4 kernel.bin=kernel.bin

clean:
	rm -f *.o bootloader.elf kernel.elf kernel.bin disk.img tools/mkfs.tfs

.PHONY: all clean
