#ifndef KCON_H
#define KCON_H

#include <stdint.h>
#include <graphics/draw.h>
#include <fs/vfs.h>

void kcon_initialize(draw_context_t *ctx);
void kcon_initialize_fs(vfs_node_t *cwd);
void kcon_keyboard_handler(uint8_t character);

int putchar(int c);

#endif