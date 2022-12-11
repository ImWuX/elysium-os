CC ?= /usr/local/x86_64elfgcc/bin/x86_64-elf-gcc
LD ?= /usr/local/x86_64elfgcc/bin/x86_64-elf-ld
ASMC ?= /bin/nasm
LDFLAGS = -nostdlib
CFLAGS = -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wno-unused-variable

# Sources
ASM_SOURCES = $(shell find src/ -type f -name '*.asm' ! -name 'bootsector.asm')
C_SOURCES = $(shell find src/ -type f -name '*.c')

# Phony Targets
.PHONY: clean

# Real Targets
src/bootsector.bin: src/bootsector.asm
	${ASMC} $< -f bin -o $@

src/bootloader.bin: ${ASM_SOURCES:.asm=.o} ${C_SOURCES:.c=.o}
	${LD} ${LDFLAGS} -o $@ -T"link.ld" $^

# Wildcard Targets
%.o: %.asm
	${ASMC} -isrc $< -f elf64 -o $@

%.o: %.c
	${CC} ${CFLAGS} -Isrc -I../libs -c $< -o $@

# TODO: Change -I../libs

# Clean Targets
clean:
	rm -f $(shell find ./src -type f -name '*.bin')
	rm -f $(shell find ./src -type f -name '*.o')