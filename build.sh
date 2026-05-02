#!/bin/bash
set -e
make CROSS_COMPILE=aarch64-linux-gnu- rockpi-s-rk3308_defconfig
make CROSS_COMPILE=aarch64-linux-gnu- \
    KCFLAGS="-fcommon" \
    HOSTCFLAGS="-Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fcommon" \
    -j$(nproc) u-boot-dtb.bin
echo "Build OK: u-boot-dtb.bin"
ls -la u-boot-dtb.bin
