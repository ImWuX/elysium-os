ifndef ARCH
    $(error ARCH is not specified)
endif

ifndef CC
	$(error CC is not specified)
endif

ifndef LD
	$(error LD is not specified)
endif

ifndef ASMC
	$(error ASMC is not specified)
endif

CFLAGS = -std=gnu2x -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -O2
CFLAGS += -Wall -Wextra -Wno-unused-variable -Wno-microsoft-anon-tag -Wvla -Wshadow
CFLAGS += -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing
CFLAGS += -D__ARCH_$(shell echo $(ARCH) | tr a-z A-Z) -D__MLIBC_ABI_ONLY
CFLAGS += -g

ASMFLAGS = -Werror

# Sources
ASM_SOURCES = $(shell find src/ -type f -name "*.asm" -not -path "src/arch/*")
ASM_SOURCES += $(shell find src/arch/$(ARCH)/* -type f -name "*.asm")
C_SOURCES = $(shell find src/ -type f -name "*.c" -not -path "src/arch/*")
C_SOURCES += $(shell find src/arch/$(ARCH)/* -type f -name "*.c")

# Phony Targets
.PHONY: clean

# Targets
ifeq ($(ARCH), amd64)
src/kernel.elf: $(ASM_SOURCES:.asm=_asm.o) $(C_SOURCES:.c=.o)
	$(LD) -o $@ -T"link.amd64.ld" $^
endif

# Wildcard Targets
%_asm.o: %.asm
	$(ASMC) $(ASMFLAGS) -isrc $< -f elf64 -o $@

%.o: %.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

# Clean Targets
clean:
	rm -f $(shell find src -type f -name "*.o")
	rm -f $(shell find src -type f -name "*.elf")