TARTARUS = .chariot-cache/built/tartarus
KERNEL = .chariot-cache/built/kernel
SYSROOT = .chariot-cache/built/root

.PHONY: clean

build:
	mkdir build

build/empty.img: build
	dd if=/dev/zero of=$@ bs=512 count=131072
# 64MiB: count=131072, 512MiB: count=1048576, 5GiB: count=10485760

build/root.rdk:
	chariot --verbose source:test test root
	cp $(SYSROOT)/root.rdk $@

build/elysium.img: build/empty.img build/root.rdk
	chariot source:kernel tartarus kernel

	cp build/empty.img $@
	sgdisk -n 1:2048:`echo $$((($$(wc -c $(TARTARUS)/tartarus.sys | awk '{print $$1}') + 512) / 512 + 2048))` $@
	sgdisk -t=1:{54524154-5241-5355-424F-4F5450415254} $@
	sgdisk -A=1:set:0 $@
	sgdisk -c=1:tartarus $@
	dd if=$(TARTARUS)/tartarus.sys of=$@ seek=2048 bs=512 conv=notrunc
	dd if=$(TARTARUS)/mbr.bin of=$@ ibs=440 seek=0 obs=1 conv=notrunc

	sgdisk -n=2:0:0 $@
	sudo losetup -Pf --show $@ >loop_device_name
	sudo mkfs.fat -F 32 `cat loop_device_name`p2
	mkdir -p loop_mount_point
	sudo mount `cat loop_device_name`p2 loop_mount_point
	sudo cp $(KERNEL)/kernel.elf support/tartarus.cfg build/root.rdk loop_mount_point/
	sudo /bin/sh -c "nm $(KERNEL)/kernel.elf -n > loop_mount_point/ksymb.txt"
	sync
	sudo umount loop_mount_point
	sudo losetup -d `cat loop_device_name`
	rm -rf loop_device_name loop_mount_point

clean:
	rm -rf build/elysium.img build/root.rdk