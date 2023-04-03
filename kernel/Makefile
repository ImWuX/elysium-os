CC = /usr/local/x86_64elfgcc/bin/x86_64-elf-gcc
LD = /usr/local/x86_64elfgcc/bin/x86_64-elf-ld
ASMC = /bin/nasm
LDFLAGS = -nostdlib
CFLAGS = -ffreestanding -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wno-unused-variable -O2

# Sources
ASM_SOURCES = $(shell find src/ -type f -name '*.asm')
C_SOURCES = $(shell find src/ -type f -name '*.c')

# Phony Targets
.PHONY: clean

# Targets
src/kernel.elf: ${ASM_SOURCES:.asm=_s.o} ${C_SOURCES:.c=.o}
	${LD} ${LDFLAGS} -o $@ -T"link.ld" $^

# Wildcard Targets
%_s.o: %.asm
	${ASMC} -isrc $< -f elf64 -o $@

%.o: %.c
	${CC} ${CFLAGS} -Isrc -Isrc/klibc -I../../tartarus-bootloader/shared -c $< -o $@

# TODO: Change -I../../tartarus-bootloader/shared

# Clean Targets
clean:
	rm -f $(shell find ./src -type f -name '*.o')
	rm -f $(shell find ./src -type f -name '*.elf')