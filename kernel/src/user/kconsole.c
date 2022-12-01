#include "kconsole.h"
#include <string.h>
#include <stdio.h>
#include <drivers/display.h>
#include <user/tgarender.h>
#include <util/vesa_font.h>
#include <util/util.h>
#include <memory/heap.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <cpu/pit.h>
#include <fs/fat32.h>

#define MAX_CHARACTERS_WRITTEN 500
#define PREFIX ">> "
#define BG 0xFF000000
#define FG 0xFFFFFFFF

static int g_width;
static int g_height;
static int g_corner_x;
static int g_corner_y;
static int g_cursor_x;
static int g_cursor_y;

static int g_characters_written;
static char *g_characters;

static void calculate_screen() {
    if(g_cursor_x >= g_width / FONT_WIDTH) {
        g_cursor_x = 0;
        g_cursor_y++;
    }
    if(g_cursor_y >= g_height / FONT_HEIGHT) {
        g_cursor_y = g_height / FONT_HEIGHT - 1;
        g_cursor_x = 0;

        draw_rect(g_corner_x, g_corner_y, g_width, g_height, BG);
        g_cursor_x = 0;
        g_cursor_y = 0;
    }
}

static void simple_print(char *str) {
    while(*str != 0) putchar(*str++);
}

static void command_handler(char *input) {
    int index = 0;
    while(input[index] != 0 && input[index] != ' ') index++;
    char* command = malloc(sizeof(char) * (index + 1));
    memcpy(input, command, index);
    command[index] = 0;

    int full_index = 0;
    while(input[full_index] != 0) full_index++;
    memcpy(input + index + 1, input, full_index);

    if(strcmp(command, "clear") == 0) {
        draw_rect(g_corner_x, g_corner_y, g_width, g_height, BG);
        g_cursor_x = 0;
        g_cursor_y = 0;
        return;
    }

    if(strcmp(command, "time") == 0) {
        printf("Time since system startup: %i\n", get_time_ms());
        return;
    }

    if(strcmp(command, "file") == 0) {
        dir_entry_t *dir = read_root_directory();
        int intlen = full_index - 1 - index;
        if(intlen <= 0) {
            while(dir != 0) {
                for(int i = 0; i < 11; i++) {
                    putchar(dir->fd.name[i]);
                }
                printf(" %i %x bytes\n", dir->fd.cluster_num, dir->fd.file_size);
                dir = dir->last_entry;
            }
        } else {
            uint64_t pow = 1;
            for(int i = 1; i < intlen; i++) pow *= 10;
            uint64_t index = 0;
            for(int i = 0; i < intlen; i++) {
                index += pow * (input[i] - '0');
                pow /= 10;
            }

            for(uint64_t i = 0; i < index; i++) {
                dir = dir->last_entry;
                if(!dir) {
                    printf("Invalid file index\n");
                    return;
                }
            }

            if(dir->fd.name[8] == 'T' && dir->fd.name[9] == 'X' && dir->fd.name[10] == 'T') {
                void *temp_buffer = malloc(dir->fd.file_size);
                fread(&dir->fd, dir->fd.file_size, temp_buffer);
                for(uint64_t i = 0; i < dir->fd.file_size; i++) {
                    putchar(((uint8_t *) temp_buffer)[i]);
                }
                free(temp_buffer);
                printf("\nDone\n");
            } else if(dir->fd.name[8] == 'T' && dir->fd.name[9] == 'G' && dir->fd.name[10] == 'A') {
                draw_image(&dir->fd, 850, 150);
            } else {
                printf("Unsupported file\n");
            }
        }
        return;
    }

    if(strcmp(command, "mm") == 0) {
        int intlen = full_index - 1 - index;
        uint64_t pow = 1;
        for(int i = 1; i < intlen; i++) pow *= 16;
        uint64_t num = 0;
        for(int i = 0; i < intlen; i++) {
            num += pow * (input[i] - '0');
            pow /= 16;
        }
        printf("%x\n", get_physical_address(num));
    }
}

void initialize_kconsole(int x, int y, int w, int h) {
    g_width = w;
    g_height = h;
    g_corner_x = x;
    g_corner_y = y;
    g_cursor_x = 0;
    g_cursor_y = 0;

    g_characters_written = 0;
    g_characters = malloc(sizeof(char) * MAX_CHARACTERS_WRITTEN);

    draw_rect(x, y, w, h, BG);
    simple_print("  _  _        _    ___  ___  \n");
    simple_print(" | \\| |___ __| |_ / _ \\/ __| \n");
    simple_print(" | .` / -_|_-<  _| (_) \\__ \\ \n");
    simple_print(" |_|\\_\\___/__/\\__|\\___/|___/ \n");
    simple_print("\nWelcome To NestOS\n\n");
    simple_print(PREFIX);
}

int putchar(int character) {
    switch(character) {
        case '\n':
            g_cursor_x = 0;
            g_cursor_y++;
            calculate_screen();
            break;
        case '\b':
            g_cursor_x--;
            if(g_cursor_x < 0) g_cursor_x = 0;
            draw_rect(g_corner_x + g_cursor_x * FONT_WIDTH, g_corner_y + g_cursor_y * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, BG);
            calculate_screen();
            break;
        default:
            draw_char(g_corner_x + g_cursor_x * FONT_WIDTH, g_corner_y + g_cursor_y * FONT_HEIGHT, character, FG, BG);
            g_cursor_x++;
            calculate_screen();
            break;
    }
    return character;
}

void keyboard_handler(uint8_t character) {
    switch(character) {
        case 0: return;
        case '\b':
            if(g_characters_written <= 0) return;
            g_characters_written--;
            break;
        case '\n':
            putchar('\n');
            g_characters[g_characters_written] = 0;
            command_handler(g_characters);
            g_characters_written = 0;
            simple_print(PREFIX);
            return;
        default:
            if(g_characters_written > MAX_CHARACTERS_WRITTEN) return;
            g_characters[g_characters_written] = character;
            g_characters_written++;
            break;
    }
    putchar(character);
}