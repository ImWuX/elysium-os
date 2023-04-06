#include "kcon.h"
#include <string.h>
#include <stdio.h>
#include <graphics/basicfont.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/pmm_lowmem.h>
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

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

static bool get_arg(char *str, int argc, char **value) {
    int cur = 0;
    while(*str && cur != argc) {
        if(is_whitespace(*str)) {
            while(is_whitespace(*str)) str++;
            cur++;
        } else str++;
    }
    if(cur != argc) return true;

    int strlen = 0;
    while(str[strlen] && str[strlen] != ' ') strlen++;
    *value = heap_alloc(strlen + 1);
    memcpy(*value, str, strlen);
    (*value)[strlen] = 0;
    return false;
}

static bool get_arg_num(char *str, int argc, uint64_t *value) {
    char *arg;
    if(get_arg(str, argc, &arg)) {
        heap_free(arg);
        return true;
    }
    int argl = strlen(arg);
    *value = 0;
    uint64_t pow = 1;
    if(arg[0] == '0' && arg[1] == 'x') {
        for(int i = argl - 1; i >= 2; i--) {
            bool valid = false;
            if(arg[i] >= '0' && arg[i] <= '9') { *value += (arg[i] - '0') * pow; valid = true; }
            if(arg[i] >= 'a' && arg[i] <= 'f') { *value += (arg[i] - 'a' + 10) * pow; valid = true; }
            if(arg[i] >= 'A' && arg[i] <= 'F') { *value += (arg[i] - 'A' + 10) * pow; valid = true; }
            if(!valid) {
                heap_free(arg);
                return true;
            }
            pow *= 16;
        }
    } else {
        for(int i = argl - 1; i >= 0; i--) {
            if(arg[i] < '0' || arg[i] > '9') {
                heap_free(arg);
                return true;
            }
            *value += ((uint64_t) arg[i] - '0') * pow;
            pow *= 10;
        }
    }
    heap_free(arg);
    return false;
}

static void command_handler(char *input) {
    char *s = input;
    bool last_was_space = false;
    int arg_count = 1;
    while(*s++) {
        if(is_whitespace(*s)) {
            if(!last_was_space) arg_count++;
            last_was_space = true;
        } else last_was_space = false;
    }

    char *command;
    get_arg(input, 0, &command);
    if(command[0]) {
        if(strcmp(command, "clear") == 0) {
            clear();
        } else if(strcmp(command, "time") == 0) {
            printf("Time since system startup: %lis\n", pit_time_s());
        } else if(strcmp(command, "ud2") == 0) {
            asm("ud2");
        } else if(strcmp(command, "timer") == 0) {
            if(configure_timer(1, 8, 100, false)) {
                printf("timer errorrrr\n");
            }
        } else if(strcmp(command, "sp") == 0) {
            uint64_t sp;
            asm volatile("mov %%rsp, %0" : "=rm" (sp));
            printf("interrupt stack pointer >> %lx\n", sp);
        } else if(strcmp(command, "cd") == 0) {
            // int arg_length = 0;
            // while(input[arg_length]) arg_length++;
            // int i = 0;
            // while(i < 512) {
            //     if(!g_path[i]) break;
            //     i++;
            // }
            // if(i + arg_length >= 512) {
            //     printf("Exceeded path limit");
            //     return;
            // }
            // for(int j = 0; j < arg_length; j++) {
            //     g_path[i + j] = input[j];
            // }
            // g_path[i + arg_length] = 0;
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
        } else if(strcmp(command, "rpage") == 0) {
            printf("Requested 1 page >> %#lx\n", pmm_page_request());
        } else if(strcmp(command, "reqcpage") == 0) {
            uint64_t pages;
            if(arg_count < 2) {
                printf("Missing arguments\n");
            } else {
                if(get_arg_num(input, 1, &pages)) {
                    printf("Invalid arguments\n");
                } else {
                    printf("Requested %li pages >> %#lx\n", pages, pmm_lowmem_request(pages));
                }
            }
        } else if(strcmp(command, "relcpage") == 0) {
            uint64_t address;
            uint64_t pages;
            if(arg_count < 3) {
                printf("Missing arguments\n");
            } else {
                if(get_arg_num(input, 1, &address) || get_arg_num(input, 2, &pages)) {
                    printf("Invalid arguments\n");
                } else {
                    pmm_lowmem_release((void *) address, pages);
                    printf("Released %li pages starting at %#lx\n", pages, address);
                }
            }
        } else if(strcmp(command, "read") == 0) {
            if(arg_count < 3) {
                printf("Missing arguments\n");
            } else {
                uint64_t sector;
                uint64_t dest;
                if(get_arg_num(input, 1, &sector) || get_arg_num(input, 2, &dest)) {
                    printf("Invalid arguments\n");
                } else {
                    ahci_read(0, sector, 1, (void *) dest);
                    printf("Read sector %li into %#lx\n", sector, dest, dest);
                }
            }
        } else if(strcmp(command, "hexdump") == 0) {
            if(arg_count < 3) {
                printf("Missing arguments\n");
            } else {
                uint64_t address;
                uint64_t count;
                if(get_arg_num(input, 1, &address) || get_arg_num(input, 2, &count)) {
                    printf("Invalid arguments");
                } else {
                    bool star = false;
                    uint8_t last[10];
                    int row_count = ((int) count + 9) / 10;
                    for(int y = 0; y < row_count; y++) {
                        int row_length = 10 - ((int) count - y * 10) % 10;
                        bool identical = (y != 0 && y != row_count - 1);
                        if(identical) {
                            for(int x = 0; x < row_length; x++) {
                                uint8_t value = *(uint8_t *) (HHDM(address) + y * 10 + x);
                                if(last[x] != value) identical = false;
                                last[x] = value;
                            }
                        }

                        if(!identical) {
                            star = false;
                            printf("%#18.16lx:  ", address + y * 10);
                            for(int x = 0; x < row_length; x++) {
                                printf("%.2x ", *(uint8_t *) (HHDM(address) + y * 10 + x));
                            }

                            printf("  ");
                            for(int x = 0; x < row_length; x++) {
                                char c = *(char *) (HHDM(address) + y * 10 + x);
                                if(c < 32 || c > 126) c = '.';
                                printf("%c ", c);
                            }
                            printf("\n");
                        } else {
                            if(star) continue;
                            printf("*\n");
                            star = true;
                        }
                    }
                }
            }
        } else if(strcmp(command, "help") == 0) {
            printf("clear: Clear the console\n");
            printf("time: Display how much time has passed since the CPU started\n");
            printf("timer: Start a timer\n");
            printf("ud2: Trigger an unknown instruction fault\n");
            printf("pcidev: Display all active PCI devices\n");
            printf("rpage: Get one page of memory\n");
            printf("rcpage: Get contiguous pages of memory\n");
            printf("sp: Display the stack pointer of an interrupt\n");
            printf("read: Loads a desired sector into a desired destination\n");
            printf("hexdump: Dumps memory at an address\n");
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
    printf("Stats:\n\tTotal: %li bytes\n\tFree: %li bytes\n\tUsed: %li bytes\n\tLowmem: %li bytes\n", pmm_mem_total(), pmm_mem_free(), pmm_mem_used(), pmm_mem_low());
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