CHARIOT_BUILT = .chariot-cache/built

.PHONY: all clean

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

clean:
	rm -rf build/elysium.img