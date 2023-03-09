BUILD = ${realpath ./build}

# Phony Targets
.PHONY: all clean $(BUILD)/libc.a $(BUILD)/kernel.elf $(BUILD)/disk.img

all: $(BUILD)/libc.a $(BUILD)/kernel.elf $(BUILD)/disk.img clean

# Real Targets
$(BUILD)/libc.a:
	@ echo "\e[33m>> Compiling Libc\e[0m"
	make -C libc src/libc.a
	cp libc/src/libc.a $@

$(BUILD)/kernel.elf:
	@ echo "\e[33m>> Compiling Kernel\e[0m"
	make -C kernel src/kernel.elf
	cp kernel/src/kernel.elf $@

$(BUILD)/disk.img: $(BUILD)/kernel.elf $(BUILD)/libc.a
	@ echo "\e[33m>> Creating Disk Image\e[0m"
	cp $(BUILD)/empty.img $@
	mkfs.fat -F 32 -n "MAIN_DRIVE" $@
	../tartarus-bootloader/deploy.sh $@
	mcopy -i $@ $(BUILD)/kernel.elf "::kernel.sys"
	mcopy -i $@ $(BUILD)/test.txt "::test.txt"
	mcopy -i $@ $(BUILD)/universe.tga "::univ.tga"
	mcopy -i $@ trtrs.cfg "::trtrs.cfg"

# Clean Targets
clean:
	make -C libc clean
	make -C kernel clean