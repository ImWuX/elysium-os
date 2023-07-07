#!/bin/bash

sim="qemu"
bios=1

while [ 1 ]; do
    echo -e "┌───────────────────────────────────┐"
    echo -ne "│ Simulator: $sim | Bios: $bios        "
    if [ "$sim" == "qemu" ]; then
    echo -ne " "
    fi
    echo -e "│\n│ 1) Cycle Simulator                │"
    echo -e "│ 2) Toggle Bios                    │"
    echo -e "│ Q) Quit                           │"
    echo -e "│ Any other key to run              │"
    echo -e "└───────────────────────────────────┘"
    read -n 1 -s -p $'\e[94m>> \e[0m' key
    clear
    case "$key" in
        "q")
            break
            ;;
        "1")
            if [ "$sim" == "qemu" ]; then
                sim="bochs"
                bios=1
            else
                sim="qemu"
            fi
            continue
            ;;
        "2")
            if [ "$sim" == "bochs" ]; then
                continue
            fi
            bios=$((1-$bios))
            continue
            ;;
    esac

    # Building OS
    if [ "$bios" -eq 1 ]; then
        make ARCH=amd64 BIOS=1
    else
        make ARCH=amd64
    fi
    if [ $? -eq 0 ]; then
        if [ "$sim" == "qemu" ]; then
            # Running QEMU
            if [ "$bios" -eq 1 ]; then
            echo -e "\e[32mRunning QEMU in VNC using BIOS\e[0m"
            qemu-system-x86_64 \
                -m 256M \
                -machine q35 \
                -drive format=raw,file=build/disk.img \
                -smp cores=4 \
                -vnc :0,websocket=on \
                -D ./log.txt -d int \
                -M smm=off \
                -k en-us \
                -serial file:/dev/stdout \
                -monitor stdio \
                -no-reboot \
                -net none
            else
            echo -e "\e[32mRunning QEMU in VNC using UEFI\e[0m"
            qemu-system-x86_64 \
                -m 256M \
                -machine q35 \
                -drive format=raw,file=build/disk.img \
                -smp cores=4 \
                -vnc :0,websocket=on \
                -D ./log.txt -d int \
                -M smm=off \
                -k en-us \
                -serial file:/dev/stdout \
                -monitor stdio \
                -no-reboot \
                -net none \
                -bios /usr/share/ovmf/OVMF.fd
            fi
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