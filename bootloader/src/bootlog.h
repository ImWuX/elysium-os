#ifndef BOOTLOG_H
#define BOOTLOG_H
#include <stdint.h>

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} boot_log_level_t;

void boot_log(char *str, boot_log_level_t level);
void boot_log_hex(uint64_t value);
void boot_log_clear();

#endif