export
OUT = ${realpath ./out}

# Toolchain
ASMC = /bin/nasm
CC = /usr/local/x86_64elfgcc/bin/x86_64-elf-gcc
LD = /usr/local/x86_64elfgcc/bin/x86_64-elf-ld
AR = /usr/local/x86_64elfgcc/bin/x86_64-elf-ar

# Phony Targets
.PHONY: all clean $(OUT)/mbr.bin $(OUT)/bootloader.bin $(OUT)/libc.a $(OUT)/kernel.elf $(OUT)/disk.img

all: $(OUT)/mbr.bin $(OUT)/bootloader.bin $(OUT)/libc.a $(OUT)/kernel.elf $(OUT)/disk.img clean

# Real Targets
$(OUT)/mbr.bin:
	@ echo "\e[33m>> Compiling Master Boot Record\e[0m"
	make -C bootloader src/mbr.bin
	cp bootloader/src/mbr.bin $@

$(OUT)/bootloader.bin:
	@ echo "\e[33m>> Compiling Bootloader\e[0m"
	make -C bootloader src/bootloader.bin
	cp bootloader/src/bootloader.bin $@

$(OUT)/libc.a:
	@ echo "\e[33m>> Compiling Libc\e[0m"
	make -C libc src/libc.a
	cp libc/src/libc.a $@

$(OUT)/kernel.elf:
	@ echo "\e[33m>> Compiling Kernel\e[0m"
	make -C kernel src/kernel.elf
	cp kernel/src/kernel.elf $@

$(OUT)/disk.img: $(OUT)/mbr.bin
	@ echo "\e[33m>> Creating Disk Image\e[0m"
	cp $(OUT)/empty.img $@
	mkfs.fat -F 32 -n "MAIN_DRIVE" $@
	dd if=$(OUT)/mbr.bin of=$@ ibs=422 seek=90 obs=1 conv=notrunc
	perl -e 'print pack("ccc",(0xEB,0x58,0x90))' | dd of=$@ bs=1 seek=0 count=3 conv=notrunc
	mcopy -i $@ $(OUT)/bootloader.bin "::bootload.sys"
	mcopy -i $@ $(OUT)/kernel.elf "::kernel.sys"
	mcopy -i $@ $(OUT)/test.txt "::test.txt"
	mcopy -i $@ $(OUT)/universe.tga "::univ.tga"

# Clean Targets
clean:
	make -C bootloader clean
	make -C libc clean
	make -C kernel clean