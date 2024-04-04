include conf.mk

DESTDIR ?= /

# Flags
CFLAGS := -std=gnu2x -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -O2
CFLAGS += -Wall -Wextra -Wvla -Wshadow
CFLAGS += -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing
CFLAGS += -D__ARCH_$(shell echo $(ARCH) | tr a-z A-Z) -D__ENV_$(shell echo $(ENV) | tr a-z A-Z) -D__MLIBC_ABI_ONLY
CFLAGS += -I$(SYSROOT)/usr/include

ASMFLAGS := -Werror

ifeq ($(ENV), dev)
CFLAGS += -g -fsanitize=undefined -fno-sanitize=alignment
ifeq ($(ARCH), x86_64)
CFLAGS += -fno-sanitize=alignment
endif
endif

# Sources
ASM_SOURCES := $(shell find $(SRC) -type f -name "*.asm" -not -path "$(SRC)/arch/*")
ASM_SOURCES += $(shell find $(SRC)/arch/$(ARCH) -type f -name "*.asm")
C_SOURCES := $(shell find $(SRC) -type f -name "*.c" -not -path "$(SRC)/arch/*")
C_SOURCES += $(shell find $(SRC)/arch/$(ARCH) -type f -name "*.c")

# Targets
.PHONY: all install clean

all: $(BUILD)/kernel.elf clean

ifeq ($(ARCH), x86_64)
$(BUILD)/kernel.elf: $(ASM_SOURCES:.asm=_asm.o) $(C_SOURCES:.c=.o) $(LIBGCC)
	mkdir -p $(BUILD)
	$(LD) -o $@ -T"$(SUPPORT)/link.x86_64.ld" $^
endif

%_asm.o: %.asm
	$(ASMC) $(ASMFLAGS) -i$(SRC) $< -f elf64 -o $@

%.o: %.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

install:
	mkdir -p $(DESTDIR)$(PREFIX)/share
	cp $(BUILD)/kernel.elf $(DESTDIR)$(PREFIX)/share

clean:
	rm -f $(shell find $(SRC) -type f -name "*.o")