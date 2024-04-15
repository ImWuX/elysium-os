#!/bin/bash
clear

make clean all

qemu_args=()
qemu_args+=(-m 256M)
qemu_args+=(-machine q35)
qemu_args+=(-cpu qemu64)
qemu_args+=(-drive format=raw,file=build/elysium.img)
qemu_args+=(-smp cores=1)
# qemu_args+=(-vnc :0,websocket=on)
qemu_args+=(-vga virtio)
qemu_args+=(-display gtk,zoom-to-fit=on,show-tabs=on,gl=on)
qemu_args+=(-D ./log.txt)
qemu_args+=(-d int)
qemu_args+=(-M smm=off)
qemu_args+=(-k en-us)
qemu_args+=(-serial file:/dev/stdout)
qemu_args+=(-monitor stdio)
qemu_args+=(-no-reboot)
qemu_args+=(-no-shutdown)
qemu_args+=(-net none)
qemu_args+=(-machine accel=kvm)

qemu-system-x86_64 "${qemu_args[@]}"
