#include "kcon.h"
#include <string.h>
#include <stdio.h>
#include <graphics/basicfont.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <drivers/pit.h>
#include <drivers/mouse.h>
#include <kdesktop.h>

#define DEFAULT_FG 0xFFFFFFFF
#define DEFAULT_BG 0
#define TAB_WIDTH 4
#define MAX_CHARACTERS 200
#define PREFIX "> "

static int g_cursor_x;
static int g_cursor_y;
static draw_context_t *g_ctx;

static char g_chars[MAX_CHARACTERS];
static int g_chars_written;
static char g_path[512];

static void simple_print(char *str) {
    while(*str) putchar(*str++);
}

static void clear() {
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, DEFAULT_BG);
    g_cursor_x = 0;
    g_cursor_y = 0;
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
        } else if(strcmp(command, "kstat") == 0) {
            kdesktop_create_window(0, 0, 300, 300, "KSTAT");
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

void kcon_initialize(draw_context_t *ctx) {
    g_ctx = ctx;
    g_chars_written = 0;
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, DEFAULT_BG);
    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n\n");
    printf("Welcome to Elysium OS\n");
    printf("Physical Memory Initialized\n");
    printf("Stats:\n\tTotal: %i bytes\n\tFree: %i bytes\n\tUsed: %i bytes\n", pmm_mem_total(), pmm_mem_free(), pmm_mem_used());
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
    if(g_ctx) {
        switch(c) {
            case '\n':
                g_cursor_x = 0;
                g_cursor_y += BASICFONT_HEIGHT;
                break;
            case '\t':
                g_cursor_x += (TAB_WIDTH - (g_cursor_x / BASICFONT_WIDTH) % TAB_WIDTH) * BASICFONT_WIDTH;
                break;
            case '\b':
                g_cursor_x -= BASICFONT_WIDTH;
                if(g_cursor_x < 0) g_cursor_x = 0;
                draw_rect(g_ctx, g_cursor_x, g_cursor_y, BASICFONT_WIDTH, BASICFONT_HEIGHT, DEFAULT_BG);
                break;
            default:
                draw_char(g_ctx, g_cursor_x, g_cursor_y, c, DEFAULT_FG);
                g_cursor_x += BASICFONT_WIDTH;
                break;
        }
        if(g_cursor_x + BASICFONT_WIDTH >= g_ctx->width) {
            g_cursor_x = 0;
            g_cursor_y += BASICFONT_HEIGHT;
        }
        if(g_cursor_y + BASICFONT_HEIGHT >= g_ctx->height) clear();
        g_ctx->invalidated = true;
    }
    return c;
}