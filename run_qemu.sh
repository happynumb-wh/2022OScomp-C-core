#!/bin/bash
# qemu-system-riscv64 \
# 	-machine virt -cpu rv64 -m 128M -smp 2\
# 	-nographic -bios ./bootloader/rustsbi-qemu \
# 	-device loader,file=./target/qemu_head.bin,addr=0x80200000 \
# 	-drive file=img/sdcard.img,if=none,format=raw,id=x0 \
# 	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
# 	-s

# 评测机
qemu-system-riscv64 \
	-machine virt \
	-kernel ./target/qemu_head.bin -m 128M \
	-nographic -smp 2 -bios ./bootloader/rustsbi-qemu \
	-drive file=./img/sdcard.img,if=none,format=raw,id=x0 \
	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 \
	-initrd ./img/sdcard.img \
	-s

# -device loader,file=./target/qemu_head.bin,addr=0x80200000 \
