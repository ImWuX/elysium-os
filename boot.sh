#!/bin/bash
POSITIONAL_ARGS=()

COLI="\e[96m"
COLE="\e[31m"
COLU="\e[4m"
COLR="\e[0m"

log() {
    echo -e "$@"
}
log_important() {
    echo -e ""$COLI"$@"$COLR""
}
log_error() {
    echo -e ""$COLE"$@"$COLR""
}

SIM="qemu"
BIOS=1

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--sim)
            SIM="$2"
            shift
            ;;
        --uefi)
            BIOS=0
            ;;
        -?|--help)
            log "┌────────────────────────────────────────────────────────────────────────"
            log "│ ElysiumOS Boot"
            log "│ Usage: ./boot [options]"
            log "│ Options:"
            log "│ \t-s, --sim <simulator>\n│ \t\t\tSpecify simulator. Supported simulator: "$COLU"qemu"$COLR", bochs\n│ "
            log "│ \t--uefi\n│ \t\t\tUse uefi to boot Elysium instead of bios\n│ "
            log "│ \t-?, --help\n│ \t\t\tDisplay usage help for Elysium boot\n│ "
            exit 1
            ;;
        -*|--*)
            log_error "Unknown option \"$1\""
            exit 1
            ;;
        *)
            POSITIONAL_ARGS+=("$1") # save positional arg
            ;;
    esac
    shift
done

set -- "${POSITIONAL_ARGS[@]}"

if [ "$BIOS" -eq 1 ]; then
    make ARCH=amd64 BIOS=1
else
    make ARCH=amd64
fi

case $SIM in
    qemu)
        # Running QEMU
        if [ "$BIOS" -eq 1 ]; then
            log_important "Running QEMU in VNC using BIOS"
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
                -gdb tcp::1234
        else
            log_important "Running QEMU in VNC using UEFI"
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
                -gdb tcp::1234 \
                -bios /usr/share/ovmf/OVMF.fd
        fi
        ;;
    bochs)
        # Running Bochs
        log_important "Running BOCHS in VNC"
        bochs -f ./conf.bxrc -q
        ;;
    *)
        log_error "Unsupported simulator ($SIM)"
        ;;
esac