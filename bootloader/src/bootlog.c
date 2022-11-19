#include "bootlog.h"
#include <stdint.h>

#define VGA_TEXT_ADDRESS 0xB8000
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_WHITE_ON_BLACK 0x0F

static int g_x = 0;
static int g_y = 0;

void boot_log(char *str, boot_log_level_t level) {
    uint8_t *buf = (uint8_t *) VGA_TEXT_ADDRESS;
    while(*str) {
        buf[(VGA_TEXT_WIDTH * g_y + g_x) * 2] = *str;
        buf[(VGA_TEXT_WIDTH * g_y + g_x) * 2 + 1] = VGA_TEXT_WHITE_ON_BLACK;
        g_x++;
        str++;
    }
    g_y++;
    g_x = 0;
}

void boot_log_clear() {
    uint16_t *buf = (uint16_t *) VGA_TEXT_ADDRESS;
    for(int i = 0; i < VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT; i++) buf[i] = 0;
}