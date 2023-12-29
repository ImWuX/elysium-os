CHARIOT_BUILT = .chariot-cache/built

.PHONY: all clean setup_dev

all: build/elysium.img

mkimg/mkimg:
	(cd mkimg && go build .)

build:
	mkdir -p build

build/elysium.img: build mkimg/mkimg
	chariot tartarus source:kernel kernel
	mkimg/mkimg \
		--bootsect=$(CHARIOT_BUILT)/tartarus/usr/share/tartarus/mbr.bin \
		--tartarus=$(CHARIOT_BUILT)/tartarus/usr/share/tartarus/tartarus.sys \
		--kernel=$(CHARIOT_BUILT)/kernel/usr/local/share/kernel.elf \
		--conf=support/tartarus.cfg $@

setup_dev:
	chariot cross-gcc tartarus

clean:
	rm -rf build/elysium.img