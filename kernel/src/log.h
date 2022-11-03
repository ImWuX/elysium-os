#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KLOG_INFO,
    KLOG_DEBUG,
    KLOG_WARN,
    KLOG_ERROR,
    KLOG_PANIC
} klog_level_t;

void klog(klog_level_t level, const char *fmt, va_list args);

#endif