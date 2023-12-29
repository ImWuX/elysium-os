#!/bin/sh

while [[ $# -gt 0 ]]; do
    case $1 in
        --libgcc=*)
            LIBGCC=${1#*=}
            ;;
        --prefix=*)
            PREFIX=${1#*=}
            ;;
        -*|--*)
            echo "Unknown option \"$1\""
            exit 1
            ;;
        *)
            POSITIONAL_ARGS+=("$1") # save positional arg
            ;;
    esac
    shift
done
set -- "${POSITIONAL_ARGS[@]}"

if [ -z "$LIBGCC" ]; then
    >&2 echo "Missing LIBGCC path"
    exit 1
fi

SRCDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
DSTDIR=$(pwd)

cp $SRCDIR/Makefile $DSTDIR
mkdir -p $DSTDIR/build

CONFMK="$DSTDIR/conf.mk"
echo "SRC := $SRCDIR/src" > $CONFMK
echo "SUPPORT := $SRCDIR/support" >> $CONFMK
echo "BUILD := $DSTDIR/build" >> $CONFMK
echo "CC := x86_64-elf-gcc" >> $CONFMK
echo "LD := x86_64-elf-ld" >> $CONFMK
echo "LIBGCC := $LIBGCC" >> $CONFMK
echo "PREFIX := $PREFIX" >> $CONFMK