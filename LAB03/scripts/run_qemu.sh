#!/bin/bash

set -e

cd "$(dirname "$0")/.."

qemu-system-arm \
    -M vexpress-a9 \
    -cpu cortex-a9 \
    -m 512M \
    -smp 2 \
    -kernel output/zImage \
    -dtb output/vexpress-v2p-ca9.dtb \
    -initrd output/initramfs.cpio.gz \
    -append "console=ttyAMA0,115200 rdinit=/sbin/init" \
    -nographic \
    -net nic \
    -net user,hostfwd=tcp::1234-:1234
