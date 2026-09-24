CC = aarch64-linux-gnu-gcc
AS = aarch64-linux-gnu-as
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

CFLAGS = -ffreestanding -nostdlib -nostartfiles -Wall -Wextra -O2 -Iinclude

all: thunderz.bin

boot.o: boot/boot.S
	$(AS) -o $@ $<

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c -o $@ $<

uart.o: kernel/uart.c
	$(CC) $(CFLAGS) -c -o $@ $<

thunderz.elf: boot.o kernel.o uart.o linker.ld
	$(LD) -T linker.ld -o $@ boot.o kernel.o uart.o

thunderz.bin: thunderz.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o thunderz.elf thunderz.bin

.PHONY: all clean