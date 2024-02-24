CFLAGS = -std=gnu2x -ffreestanding -mcmodel=kernel -mno-red-zone -mgeneral-regs-only -O2
CFLAGS += -Wall -Wextra -Wvla -Wshadow -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter
CFLAGS += -fno-stack-protector -fno-stack-check -fno-omit-frame-pointer -fno-strict-aliasing

init: src/main.c
	x86_64-elysium-gcc $(CFLAGS) $^ -o $@