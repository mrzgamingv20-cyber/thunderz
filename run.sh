#!/bin/bash
# thunderz OS runner for Termux

set -e

ELF="thunderz.elf"

if [ ! -f "$ELF" ]; then
    echo "Error: $ELF not found"
    echo "Download from GitHub Actions artifacts first"
    exit 1
fi

echo "Starting thunderz on QEMU..."
echo "Press Ctrl+A then X to exit"
echo ""

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -bios "$ELF" \
    -m 128M \
    -nographic \
    -serial mon:stdio