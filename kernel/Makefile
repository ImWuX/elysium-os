CC = /usr/local/x86_64elfgcc/bin/x86_64-elf-gcc
LD = /usr/local/x86_64elfgcc/bin/x86_64-elf-ld
ASMC = /bin/nasm
CFLAGS = -std=gnu2x -ffreestanding -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wno-unused-variable -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -O2 -D __ARCH_${ARCH}

# Sources
ASM_SOURCES = $(shell find src/ -type f -name '*.asm' -not -path "src/arch/*")
ASM_SOURCES += $(shell find src/arch/$(shell echo $(ARCH) | tr A-Z a-z)/* -type f -name '*.asm')
C_SOURCES = $(shell find src/ -type f -name '*.c' -not -path "src/arch/*")
C_SOURCES += $(shell find src/arch/$(shell echo $(ARCH) | tr A-Z a-z)/* -type f -name '*.c')

# Phony Targets
.PHONY: clean

# Targets
src/kernel.elf: ${ASM_SOURCES:.asm=_s.o} ${C_SOURCES:.c=.o}
	${LD} -o $@ -T"link.ld" $^

# Wildcard Targets
%_s.o: %.asm
	${ASMC} -isrc $< -f elf64 -o $@

%.o: %.c
	${CC} ${CFLAGS} -Isrc -Isrc/klibc -I../../tartarus-bootloader -c $< -o $@

# TODO: Change -I../../tartarus-bootloader

# Clean Targets
clean:
	rm -f $(shell find ./src -type f -name '*.o')
	rm -f $(shell find ./src -type f -name '*.elf')