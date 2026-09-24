#!/bin/bash
# thunderz OS runner for Termux

set -e

BIN="thunderz.bin"

if [ ! -f "$BIN" ]; then
    echo "Error: $BIN not found"
    echo "Download from GitHub Actions artifacts first"
    exit 1
fi

echo "Starting thunderz on QEMU..."
echo "Press Ctrl+A then X to exit"
echo ""

qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -kernel "$BIN" \
    -m 128M \
    -nographic \
    -serial mon:stdio