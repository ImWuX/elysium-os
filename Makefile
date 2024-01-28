CHARIOT_BUILT = .chariot-cache/built

.PHONY: all clean setup_dev

all: build/elysium.img

mkimg/mkimg:
	(cd mkimg && go build .)

build:
	mkdir -p build

build/kernel.elf: build
	chariot source:kernel kernel
	cp $(CHARIOT_BUILT)/kernel/usr/local/share/kernel.elf $@

build/kernsymb.txt: build/kernel.elf
	nm build/kernel.elf -n > $@

build/elysium.img: build mkimg/mkimg build/kernel.elf build/kernsymb.txt
	chariot tartarus
	mkimg/mkimg \
		--bootsect=$(CHARIOT_BUILT)/tartarus/usr/share/tartarus/mbr.bin \
		--tartarus=$(CHARIOT_BUILT)/tartarus/usr/share/tartarus/tartarus.sys \
		--kernel=build/kernel.elf \
		--files=support/tartarus.cfg,build/kernsymb.txt $@

setup_dev:
	chariot host:cross-gcc tartarus

clean:
	rm -rf build/*