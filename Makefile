BUILD = ${realpath ./build}
FILES = ${realpath ./files}
TARTARUS = ${realpath ../tartarus-bootloader/}
ARCH ?= AMD64

# Phony Targets
.PHONY: all clean $(BUILD)/kernel.elf $(BUILD)/ksymbols.txt $(BUILD)/disk.img

all: $(BUILD)/kernel.elf $(BUILD)/disk.img clean

# Real Targets
$(BUILD)/kernel.elf:
	@ echo "\e[33m>> Compiling Kernel\e[0m"
	$(MAKE) -C kernel src/kernel.elf
	cp kernel/src/kernel.elf $@

$(BUILD)/ksymbols.txt: $(BUILD)/kernel.elf
	nm $(BUILD)/kernel.elf -n > $(BUILD)/ksymbols.txt

$(BUILD)/disk.img: $(BUILD)/kernel.elf $(BUILD)/ksymbols.txt
	@ echo "\e[33m>> Creating Disk Image\e[0m"
	cp $(BUILD)/empty.img $@
ifeq ($(ARCH), AMD64)
ifdef BIOS
	$(MAKE) -C $(TARTARUS)/core tartarus.sys TARGET=amd64-bios
	sgdisk -n=1:2048:`echo $$((($$(wc -c $(TARTARUS)/core/tartarus.sys | awk '{print $$1}') + 512) / 512 + 2048))` $@
	sgdisk -t=1:{54524154-5241-5355-424F-4F5450415254} $@
	sgdisk -A=1:set:0 $@
	dd if=$(TARTARUS)/core/tartarus.sys of=$@ seek=2048 bs=512 conv=notrunc
	$(MAKE) -C $(TARTARUS)/bios disk/mbr.bin
	dd if=$(TARTARUS)/bios/disk/mbr.bin of=$@ ibs=440 seek=0 obs=1 conv=notrunc
	$(MAKE) -C $(TARTARUS)/core clean TARGET=amd64-bios
	$(MAKE) -C $(TARTARUS)/bios clean
	sgdisk -n=2:0:0 $@
	sudo losetup -Pf --show $@ >loop_device_name
	sudo mkfs.fat -F 32 `cat loop_device_name`p2
	mkdir -p loop_mount_point
	sudo mount `cat loop_device_name`p2 loop_mount_point
else
	sgdisk -n=1:0:0 $@
	sgdisk -t=1:{C12A7328-F81F-11D2-BA4B-00A0C93EC93B} $@
	sudo losetup -Pf --show $@ >loop_device_name
	sudo mkfs.fat -F 32 `cat loop_device_name`p1
	mkdir -p loop_mount_point
	sudo mount `cat loop_device_name`p1 loop_mount_point
	sudo mkdir -p loop_mount_point/EFI/BOOT
	$(MAKE) -C $(TARTARUS)/core tartarus.efi TARGET=amd64-uefi64
	sudo cp $(TARTARUS)/core/tartarus.efi loop_mount_point/EFI/BOOT/BOOTX64.EFI
	$(MAKE) -C $(TARTARUS)/core clean TARGET=amd64-uefi64
endif
endif
	sudo cp $(BUILD)/kernel.elf $(BUILD)/ksymbols.txt $(FILES)/* loop_mount_point/
	sync
	sudo umount loop_mount_point
	sudo losetup -d `cat loop_device_name`
	rm -rf loop_device_name loop_mount_point

# Clean Targets
clean:
	$(MAKE) -C kernel clean