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
UEFI=0
DEBUG=0
NOCOMP=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--sim)
            SIM="$2"
            shift
            ;;
        --uefi)
            UEFI=1
            ;;
        --debug)
            DEBUG=1
            ;;
        --nocomp)
            NOCOMP=1
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

if [ "$NOCOMP" -eq 0 ]; then
    if [ "$UEFI" -eq 0 ]; then
        make ARCH=amd64 BIOS=1
    else
        make ARCH=amd64
    fi
fi

case $SIM in
    qemu)
        # Running QEMU
        log_important "Running QEMU in VNC using BIOS"
        qemu_args=()
        qemu_args+=(-m 256M)
        qemu_args+=(-machine q35)
        qemu_args+=(-drive format=raw,file=build/disk.img)
        qemu_args+=(-smp cores=4)
        qemu_args+=(-vnc :0,websocket=on)
        qemu_args+=(-D ./log.txt)
        qemu_args+=(-d int)
        qemu_args+=(-M smm=off)
        qemu_args+=(-k en-us)
        qemu_args+=(-serial file:/dev/stdout)
        qemu_args+=(-monitor stdio)
        qemu_args+=(-no-reboot)
        qemu_args+=(-net none)
        [[ "$UEFI" -eq 1 ]] && qemu_args+=(-bios /usr/share/ovmf/x64//OVMF.fd)
        [[ "$DEBUG" -eq 1 ]] && qemu_args+=(-s -S)

        qemu-system-x86_64 "${qemu_args[@]}"
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