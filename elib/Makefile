CFLAGS = -std=gnu2x -I. -Wall -Wextra

libelib.a: src/elib.o
	x86_64-elysium-ar rcs $@ $^

%.o: %.c
	x86_64-elysium-gcc $(CFLAGS) -c $< -o $@