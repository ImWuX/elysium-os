#include "kcon.h"
#include <string.h>
#include <stdio.h>
#include <graphics/basicfont.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <drivers/pit.h>
#include <drivers/mouse.h>
#include <drivers/hpet.h>
#include <kdesktop.h>
#include <fs/vfs.h>
#include <drivers/pci.h>
#include <drivers/ahci.h>

#define DEFAULT_FG 0xFFFFFFFF
#define DEFAULT_BG 0
#define TAB_WIDTH 4
#define MAX_CHARACTERS 200
#define PREFIX '>'

static int g_cursor_x;
static int g_cursor_y;
static draw_context_t *g_ctx;

static char g_chars[MAX_CHARACTERS];
static int g_chars_written;
static char g_path[512];

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
        } else if(strcmp(command, "timer") == 0) {
            if(configure_timer(1, 8, 100, false)) {
                printf("timer errorrrr\n");
            }
        } else if(strcmp(command, "sp") == 0) {
            uint64_t sp;
            asm volatile("mov %%rsp, %0" : "=rm" (sp));
            printf("interrupt stack pointer >> %x\n", sp);
        } else if(strcmp(command, "cd") == 0) {
            int arg_length = 0;
            while(input[arg_length]) arg_length++;
            int i = 0;
            while(i < 512) {
                if(!g_path[i]) break;
                i++;
            }
            if(i + arg_length >= 512) {
                printf("Exceeded path limit");
                return;
            }
            for(int j = 0; j < arg_length; j++) {
                g_path[i + j] = input[j];
            }
            g_path[i + arg_length] = 0;
        } else if(strcmp(command, "pcidev") == 0) {
            pci_device_t *device = g_pci_devices;
            while(device) {
                uint16_t vendor_id = pci_config_read_word(device, __builtin_offsetof(pci_device_header_t, vendor_id));
                uint8_t class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, class));
                uint8_t sub_class = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, sub_class));
                uint8_t prog_if = pci_config_read_byte(device, __builtin_offsetof(pci_device_header_t, program_interface));
                if(device->segment > 0) printf("Seg %i ", device->segment);
                printf("%i:%i.%i Vendor: %x, Class: %x, SubClass: %x, ProgIf: %x\n", device->bus, device->slot, device->func, vendor_id, class, sub_class, prog_if);
                device = device->list;
            }
        } else if(strcmp(command, "request_page") == 0) {
            printf("Requested 1 page >> %x\n", pmm_page_request());
        } else if(strcmp(command, "read") == 0) {
            void *mem = pmm_page_request();
            ahci_read(0, 0, 1, mem);
            printf("Read the bootsector into %x >> signature(%x)\n", mem, *(uint16_t *) HHDM(mem + 510));
        } else if(strcmp(command, "help") == 0) {
            printf("clear: Clear the console\n");
            printf("time: Display how much time has passed since the CPU started\n");
            printf("timer: Start a timer\n");
            printf("ud2: Trigger an unknown instruction fault\n");
            printf("pcidev: Display all active PCI devices\n");
            printf("request_page: Get one page of memory\n");
            printf("sp: Display the stack pointer of an interrupt\n");
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
    g_path[0] = '/';
    g_path[1] = 0;

    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, DEFAULT_BG);
    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n\n");
    printf("Welcome to Elysium OS\n");
    printf("Physical Memory Initialized\n");
    printf("Stats:\n\tTotal: %i bytes\n\tFree: %i bytes\n\tUsed: %i bytes\n\tLowmem: %i bytes\n", pmm_mem_total(), pmm_mem_free(), pmm_mem_used(), pmm_mem_low());
    printf("%s %c ", g_path, PREFIX);
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
            printf("%s %c ", g_path, PREFIX);
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