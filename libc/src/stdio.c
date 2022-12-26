#include "stdio.h"
#include <stdarg.h>
#include <math.h>

#define STATE_NORMAL 0
#define STATE_ARG 1

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    uint8_t state = STATE_NORMAL;
    while(*fmt) {
        switch(state) {
            case STATE_NORMAL:
                if(*fmt == '%') {
                    state = STATE_ARG;
                    break;
                }
                putchar(*fmt);
                break;
            case STATE_ARG:
                switch(*fmt) {
                    case 'c':
                        putchar(va_arg(args, uint64_t));
                        break;
                    case 's': {
                        const char *str = va_arg(args, const char *);
                        while(*str) {
                            putchar(*str);
                            str++;
                        }
                    } break;
                    case 'i':
                    case 'n': {
                        int64_t value = va_arg(args, int64_t);
                        int64_t pw = 1;
                        while((value > 0 && value / pw >= 10) || (value < 0 && value / pw <= -10)) pw *= 10;

                        if(value < 0) {
                            putchar('-');
                        }

                        while(pw > 0) {
                            putchar(abs(value / pw) + '0');
                            value %= pw;
                            pw /= 10;
                        }
                    } break;
                    case 'x': {
                        // TODO: Check if I fixed this (should be div by 16?)
                        uint64_t value = va_arg(args, uint64_t);
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
                    } break;
                    case '%':
                        putchar('%');
                        break;
                }
                state = STATE_NORMAL;
                break;
        }

        fmt++;
    }

    va_end(args);
}

void printe(const char *msg) {
    printf("ERROR: %s\n", msg);
}