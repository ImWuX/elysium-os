#!/bin/bash

BINUTILS_VERSION="2.40"
GCC_VERSION="13.1.0"

export TARGET="x86_64-elf"
export PREFIX="/usr/local/x86_64elfgcc"
export PATH="$PREFIX/bin:$PATH"

# Create TMP Directory + Build Directories
mkdir tmp
cd tmp
mkdir gcc-build
mkdir binutils-build



# Download + Unzip GCC & Binutils
curl -O https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz
tar xvzf gcc-$GCC_VERSION.tar.gz
curl -O https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz
tar xvzf binutils-$BINUTILS_VERSION.tar.gz



# Configure & Install Binutils
cd binutils-build
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix=$PREFIX --disable-nls --disable-werror --with-sysroot
make all install
cd ..



# Download GCC Prerequisites
cd gcc-$GCC_VERSION
./contrib/download_prerequisites

# Build Configure
cd ../gcc-build
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers

# Redzone Config
touch ../gcc-$GCC_VERSION/gcc/config/i386/t-x86_64-elf
echo "MULTILIB_OPTIONS += mno-red-zone" >> ../gcc-$GCC_VERSION/gcc/config/i386/t-x86_64-elf
echo "MULTILIB_DIRNAMES += no-red-zone" >> ../gcc-$GCC_VERSION/gcc/config/i386/t-x86_64-elf
perl -pe 'BEGIN{undef $/;} s/(x86_64-\*-elf\*\))(\n\ttm_file=".+?"\n\t;;)/\1\n\ttmake_file="\${tmake_file} i386\/t-x86_64-elf"\2/smg' ../gcc-$GCC_VERSION/gcc/config.gcc > noredzone_config_tmpfile.txt
cp noredzone_config_tmpfile.txt ../gcc-$GCC_VERSION/gcc/config.gcc

# Build GCC
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc



# Remove TMP
cd ../..
rm -r tmp
