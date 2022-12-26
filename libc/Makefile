CC = /usr/local/x86_64elfgcc/bin/x86_64-elf-gcc
AR = /usr/local/x86_64elfgcc/bin/x86_64-elf-ar
CFLAGS = -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Wno-unused-variable

# Sources
C_SOURCES = $(shell find src/ -type f -name '*.c')

# Phony Targets
.PHONY: clean

# Targets
src/libc.a: ${C_SOURCES:.c=.o}
	${AR} -rc $@ $^

# Wildcard Targets
%.o: %.c
	${CC} ${CFLAGS} -c -Isrc $< -o $@

# Clean Targets
clean:
	rm -f $(shell find ./src -type f -name '*.o')
	rm -f $(shell find ./src -type f -name '*.a')