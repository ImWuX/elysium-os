BUILD = ${realpath ./build}

# Phony Targets
.PHONY: all clean $(BUILD)/kernel.elf $(BUILD)/ksymbols.txt $(BUILD)/disk.img

all: $(BUILD)/kernel.elf $(BUILD)/disk.img clean

# Real Targets
$(BUILD)/kernel.elf:
	@ echo "\e[33m>> Compiling Kernel\e[0m"
	make -C kernel src/kernel.elf
	cp kernel/src/kernel.elf $@

$(BUILD)/ksymbols.txt: $(BUILD)/kernel.elf
	nm $(BUILD)/kernel.elf -n > $(BUILD)/ksymbols.txt

$(BUILD)/disk.img: $(BUILD)/kernel.elf $(BUILD)/ksymbols.txt
	@ echo "\e[33m>> Creating Disk Image\e[0m"
	cp $(BUILD)/empty.img $@
	mkfs.fat -F 32 -n "MAIN_DRIVE" $@
	../tartarus-bootloader/deploy.sh $@
	mcopy -i $@ $(BUILD)/kernel.elf "::kernel.sys"
	mcopy -i $@ $(BUILD)/ksymbols.txt "::ksymbols.txt"
	mcopy -i $@ $(BUILD)/test.txt "::test.txt"
	mcopy -i $@ $(BUILD)/universe.tga "::univ.tga"
	mcopy -i $@ trtrs.cfg "::trtrs.cfg"
	mcopy -i $@ userspace/test_program "::test_program"

# Clean Targets
clean:
	make -C kernel clean