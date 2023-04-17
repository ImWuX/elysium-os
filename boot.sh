#!/bin/bash

sim="qemu"

while [ 1 ]; do
    read -n 1 -s -p $'\e[94mPress any key to continue. 1 for QEMU. 2 for BOCHS. Q to quit.\e[0m' key
    if [ "$key" == "q" ]; then
        clear
        break
    fi
    if [ "$key" == "1" ]; then
        sim="qemu"
    fi
    if [ "$key" == "2" ]; then
        sim="bochs"
    fi
    clear

    # Building OS
    make ARCH=AMD64
    if [ $? -eq 0 ]; then
        if [ "$sim" == "qemu" ]; then
            # Running QEMU
            echo -e "\e[32mRunning QEMU in VNC\e[0m"
            qemu-system-x86_64 \
                -D ./log.txt -d int \
                -m 256M \
                -drive format=raw,file=build/disk.img \
                -vnc :0,websocket=on \
                -machine q35 \
                -k en-us \
                -serial stdio \
                -s
        else
            # Running Bochs
            echo -e "\e[32mRunning BOCHS in VNC\e[0m"
            bochs -f ./conf.bxrc -q
        fi
    else
        # Cleanup after fail
        echo -e "\e[31mMake failed. Cleaning up\e[0m"
        make clean
    fi
done