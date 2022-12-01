#include "bootlog.h"

#define VGA_TEXT_ADDRESS 0xB8000
#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_WHITE_ON_BLACK 0x0F

static int g_x = 0;
static int g_y = 0;

static void putchar(char c) {
    uint8_t *buf = (uint8_t *) VGA_TEXT_ADDRESS;
    buf[(VGA_TEXT_WIDTH * g_y + g_x) * 2] = c;
    buf[(VGA_TEXT_WIDTH * g_y + g_x) * 2 + 1] = VGA_TEXT_WHITE_ON_BLACK;
    g_x++;
}

void boot_log(char *str, boot_log_level_t level) {
    while(*str) {
        switch(*str) {
            case '\n':
                g_x = 0;
                g_y++;
                break;
            default:
                putchar(*str);
                break;
        }
        str++;
    }
}

static int abs(int n) {
    return n < 0 ? -n : n;
}

void boot_log_hex(uint64_t value) {
    uint64_t pw = 1;
    while(value / pw >= 16) pw *= 16;

    putchar('0');
    putchar('x');
    while(pw > 0) {
        uint8_t c = abs(value / pw);
        if(c >= 10) {
            putchar(c - 10 + 'a');
        } else {
            putchar(c + '0');
        }
        value %= pw;
        pw /= 16;
    }
}

void boot_log_clear() {
    uint16_t *buf = (uint16_t *) VGA_TEXT_ADDRESS;
    for(int i = 0; i < VGA_TEXT_WIDTH * VGA_TEXT_HEIGHT; i++) buf[i] = 0;
}