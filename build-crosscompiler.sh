#!/bin/bash

export TARGET="x86_64-elf"
export PREFIX="/usr/local/x86_64elfgcc"
export PATH="$PREFIX/bin:$PATH"

# Create TMP Directory + Build Directories
mkdir tmp
cd tmp
mkdir gcc-build
mkdir binutils-build



# Download + Unzip GCC & Binutils
curl -O https://ftp.gnu.org/gnu/gcc/gcc-13.1.0/gcc-13.1.0.tar.gz
tar xvzf gcc-13.1.0.tar.gz
curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.gz
tar xvzf binutils-2.40.tar.gz



# Configure & Install Binutils
cd binutils-build
../binutils-2.40/configure --target=$TARGET --prefix=$PREFIX --disable-nls --disable-werror --with-sysroot
make all install
cd ..



# Download GCC Prerequisites
cd gcc-13.1.0
./contrib/download_prerequisites

# Build Configure
cd ../gcc-build
../gcc-13.1.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers

# Redzone Config
touch ../gcc-13.1.0/gcc/config/i386/t-x86_64-elf
echo "MULTILIB_OPTIONS += mno-red-zone" >> ../gcc-13.1.0/gcc/config/i386/t-x86_64-elf
echo "MULTILIB_DIRNAMES += no-red-zone" >> ../gcc-13.1.0/gcc/config/i386/t-x86_64-elf
perl -pe 'BEGIN{undef $/;} s/(x86_64-\*-elf\*\))(\n\ttm_file=".+?"\n\t;;)/\1\n\ttmake_file="\${tmake_file} i386\/t-x86_64-elf"\2/smg' ../gcc-13.1.0/gcc/config.gcc > noredzone_config_tmpfile.txt
cp noredzone_config_tmpfile.txt ../gcc-13.1.0/gcc/config.gcc

# Build GCC
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc



# Remove TMP
cd ../..
rm -r tmp
