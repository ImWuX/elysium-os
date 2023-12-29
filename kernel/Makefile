include conf.mk

ifndef DESTDIR
DESTDIR=/
endif

# Flags
CFLAGS = -std=gnu2x -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -O2
CFLAGS += -Wall -Wextra -Wno-unused-variable -Wno-microsoft-anon-tag -Wvla -Wshadow
CFLAGS += -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing
CFLAGS += -D__ARCH_$(shell echo $(ARCH) | tr a-z A-Z) -D__MLIBC_ABI_ONLY
CFLAGS += -g

ASMFLAGS = -Werror

# Sources
ASM_SOURCES = $(shell find $(SRC) -type f -name "*.asm" -not -path "arch/*")
ASM_SOURCES += $(shell find $(SRC)/arch/$(ARCH) -type f -name "*.asm")
C_SOURCES = $(shell find $(SRC) -type f -name "*.c" -not -path "arch/*")
C_SOURCES += $(shell find $(SRC)/arch/$(ARCH) -type f -name "*.c")

# Targets
.PHONY: all install

all: $(BUILD)/kernel.elf

$(BUILD)/kernel.elf: $(ASM_SOURCES:.asm=_asm.o) $(C_SOURCES:.c=.o) $(LIBGCC)
	$(LD) -o $@ -T"$(SUPPORT)/link.amd64.ld" $^

%_asm.o: %.asm
	$(ASMC) $(ASMFLAGS) -isrc $< -f elf64 -o $@

%.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

install:
	mkdir -p $(DESTDIR)$(PREFIX)/share
	cp $(BUILD)/kernel.elf $(DESTDIR)$(PREFIX)/share