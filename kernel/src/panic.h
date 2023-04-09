#ifndef PANIC_H
#define PANIC_H

#include <stdnoreturn.h>
#include <graphics/draw.h>

void panic_initialize(draw_context_t *ctx, char *symbols, uint64_t symbols_size);
noreturn void panic(char *location, char *msg);

#endif