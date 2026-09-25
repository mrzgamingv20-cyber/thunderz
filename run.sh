#!/bin/bash
# thunderz OS runner for Termux

set -e

ELF="thunderz.elf"

if [ ! -f "$ELF" ]; then
    echo "Error: $ELF not found"
    echo "Download from GitHub Actions artifacts first"
    exit 1
fi

if [ ! -f "disk.img" ]; then
    echo "Creating disk.img..."
    make disk.img 2>/dev/null || {
        echo "Need mkfs.tfs. Building..."
        gcc -O2 -o tools/mkfs.tfs tools/mkfs_tfs.c
        ./tools/mkfs.tfs disk.img 4
    }
fi

echo "Starting thunderz on QEMU..."
echo "Press Ctrl+A then X to exit"
echo ""

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -kernel "$ELF" \
    -m 128M \
    -nographic \
    -serial mon:stdio \
    -drive file=disk.img,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0