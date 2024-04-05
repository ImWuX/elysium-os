CHARIOT_BUILT = .chariot-cache/built
ROOT = $(CHARIOT_BUILT)/root/root.rdk
TARTARUS = $(CHARIOT_BUILT)/tartarus
KERNEL = $(CHARIOT_BUILT)/kernel

.PHONY: all clean setup_dev $(ROOT) $(KERNEL)

all: build/elysium.img

build:
	mkdir -p build

$(KERNEL):
	chariot source:kernel kernel

$(ROOT):
	chariot source:init init root

$(TARTARUS):
	chariot tartarus

build/kernelsymbols.txt: $(KERNEL)
	nm $(KERNEL)/usr/local/share/kernel.elf -n > $@

build/elysium.img: build build/kernelsymbols.txt $(ROOT) $(TARTARUS) $(KERNEL)
	mkimg --config=support/mkimg.toml

setup_dev:
	chariot host:cross-gcc host:gcc tartarus kernel-headers

clean:
	rm -rf build/*