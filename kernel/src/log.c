#include "log.h"
#include <stdio.h>

void klog(klog_level_t level, const char *fmt, va_list args) {
    switch(level) {
        case KLOG_INFO:
            printf("INFO | ");
            break;
        case KLOG_DEBUG:
            printf("DEBUG | ");
            break;
        case KLOG_WARN:
            printf("WARN | ");
            break;
        case KLOG_ERROR:
            printf("ERROR | ");
            break;
        case KLOG_PANIC:
            printf("PANIC | ");
            break;
    }
    printf(fmt, args);
}