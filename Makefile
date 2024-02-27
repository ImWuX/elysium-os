CHARIOT_BUILT = .chariot-cache/built
ROOT = $(CHARIOT_BUILT)/root/root.rdk
TARTARUS = $(CHARIOT_BUILT)/tartarus

.PHONY: all clean setup_dev $(ROOT)

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

$(ROOT):
	chariot source:init init root

$(TARTARUS):
	chariot tartarus

build/elysium.img: build mkimg/mkimg build/kernel.elf build/kernsymb.txt $(ROOT) $(TARTARUS)
	mkimg/mkimg \
		--bootsect=$(TARTARUS)/usr/share/tartarus/mbr.bin \
		--tartarus=$(TARTARUS)/usr/share/tartarus/tartarus.sys \
		--kernel=build/kernel.elf \
		--files=support/tartarus.cfg,build/kernsymb.txt,$(ROOT) \
		$@

setup_dev:
	chariot host:cross-gcc host:gcc tartarus kernel-headers

clean:
	rm -rf build/*