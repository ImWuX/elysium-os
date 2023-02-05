#include "kcon.h"
#include <string.h>
#include <stdio.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>
#include <memory/heap.h>
#include <drivers/pit.h>

#define DEFAULT_FG 0xFFFFFFFF
#define DEFAULT_BG 0
#define TAB_WIDTH 4
#define MAX_CHARACTERS 200
#define PREFIX "> "
#define BORDERWIDTH 5

static int g_width;
static int g_height;
static int g_cx;
static int g_cy;
static int g_x;
static int g_y;
static char g_chars[MAX_CHARACTERS];
static int g_chars_written;
static char g_path[512];

static void simple_print(char *str) {
    while(*str) putchar(*str++);
}

static void clear() {
    draw_rect(g_cx, g_cy, g_width, g_height, DEFAULT_BG);
    g_y = g_cy;
    g_x = g_cx;
}

static void command_handler(char *input) {
    int index = 0;
    while(input[index] && input[index] != ' ') index++;
    char* command = heap_alloc(sizeof(char) * (index + 1));
    memcpy(command, input, index);
    command[index] = 0;

    int full_index = 0;
    while(input[full_index]) full_index++;
    memcpy(input, input + index + 1, full_index);

    if(index != 0) {
        if(strcmp(command, "clear") == 0) {
            clear();
        } else if(strcmp(command, "time") == 0) {
            printf("Time since system startup: %is\n", pit_time_s());
        } else if(strcmp(command, "ud2") == 0) {
            asm("ud2");
        } else if(strcmp(command, "help") == 0) {
            printf("clear: Clear the console\n");
            printf("time: Display how much time has passed since the CPU started\n");
            printf("ud2: Trigger an unknown instruction fault\n");
            printf("help: Display all the commands\n");
        } else {
            printf("Unknown command: \"%s\"\n", command);
        }
    }

    heap_free(command);
}

void kcon_initialize(int width, int height, int x, int y) {
    g_width = width;
    g_height = height;
    g_cx = x;
    g_cy = y;
    g_x = g_cx;
    g_y = g_cy;
    g_chars_written = 0;
    draw_rect(x - BORDERWIDTH, y - BORDERWIDTH, width + BORDERWIDTH * 2, height + BORDERWIDTH * 2, DEFAULT_FG);
    draw_rect(x - 4, y - 4, width + 8, height + 8, DEFAULT_BG);

    int r = 100;
    int b = 150;
    int rv = 1;
    int bv = 1;
    for(uint16_t yy = 0; yy < draw_scrh(); yy++) {
        r += rv;
        b += bv;
        if(r == 200 || r == 100) rv *= -1;
        if(b == 250 || b == 150) bv *= -1;

        if(yy < y - BORDERWIDTH || yy > y + height + BORDERWIDTH) {
            draw_rect(0, yy, draw_scrw(), 1, draw_color(r, 0, b));
        } else {
            draw_rect(0, yy, x - BORDERWIDTH, 1, draw_color(r, 0, b));
            draw_rect(x + width + BORDERWIDTH, yy, draw_scrw() - x - BORDERWIDTH - width, 1, draw_color(r, 0, b));
        }
    }
}

void kcon_print_prefix() {
    simple_print(PREFIX);
}

void kcon_keyboard_handler(uint8_t character) {
    switch(character) {
        case 0: return;
        case '\b':
            if(g_chars_written <= 0) return;
            g_chars_written--;
            break;
        case '\n':
            putchar('\n');
            g_chars[g_chars_written] = 0;
            command_handler(g_chars);
            g_chars_written = 0;
            simple_print(PREFIX);
            return;
        default:
            if(g_chars_written > MAX_CHARACTERS) return;
            g_chars[g_chars_written] = character;
            g_chars_written++;
            break;
    }
    putchar(character);
}

int putchar(int c) {
    switch(c) {
        case '\n':
            g_x = g_cx;
            g_y += BASICFONT_HEIGHT;
            break;
        case '\t':
            g_x += (TAB_WIDTH - (g_x / BASICFONT_WIDTH) % TAB_WIDTH) * BASICFONT_WIDTH;
            break;
        case '\b':
            g_x -= BASICFONT_WIDTH;
            if(g_x < g_cx) g_x = g_cx;
            draw_rect(g_x, g_y, BASICFONT_WIDTH, BASICFONT_HEIGHT, DEFAULT_BG);
            break;
        default:
            draw_char(g_x, g_y, c, DEFAULT_FG, DEFAULT_BG);
            g_x += BASICFONT_WIDTH;
            break;
    }
    if(g_x + BASICFONT_WIDTH >= g_cx + g_width) {
        g_x = g_cx;
        g_y += BASICFONT_HEIGHT;
    }
    if(g_y + BASICFONT_HEIGHT >= g_cy + g_height) clear();
    return c;
}