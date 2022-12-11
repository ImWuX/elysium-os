#!/bin/bash

if [ $1 = "-bootonly" ]; then
    bootonly=true
fi

while [ 1 ]; do
    read -n 1 -s -p $'\e[94mPress any key to continue. Q to quit.\e[0m' key
    if [ "$key" == "q" ]; then
        clear
        break
    fi
    clear

    # Building OS
    if $bootonly; then
        make bootloader
    else
        make
    fi
    if [ $? -eq 0 ]; then
        # Running QEMU
        echo -e "\e[32mRunning QEMU in VNC\e[0m"
        qemu-system-x86_64 \
            -m 256M \
            -drive format=raw,file=out/disk.img \
            -vnc :0,websocket=on \
            -machine q35 \
            -k en-us \
            -serial stdio \
            -s
    else
        # Cleanup after fail
        echo -e "\e[31mMake failed. Cleaning up\e[0m"
        make clean
    fi
done