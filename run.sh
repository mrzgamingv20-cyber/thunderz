#!/bin/bash
# thunderz OS runner for Termux

set -e

ELF="bootloader.elf"

if [ ! -f "$ELF" ]; then
    echo "Error: $ELF not found"
    echo "Download from GitHub Actions artifacts first"
    exit 1
fi

if [ ! -f "disk.img" ]; then
    echo "Creating disk.img..."
    if [ ! -f "kernel.bin" ]; then
        echo "Error: kernel.bin not found (needed on disk)"
        exit 1
    fi
    make disk.img 2>/dev/null || {
        echo "Need mkfs.tfs. Building..."
        gcc -O2 -o tools/mkfs.tfs tools/mkfs_tfs.c
        ./tools/mkfs.tfs disk.img 4 kernel.bin=kernel.bin
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
    -drive file=disk.img,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0
