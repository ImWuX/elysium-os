#include "log.h"
#include <lib/format.h>

static spinlock_t g_lock = SPINLOCK_INIT;
static list_t g_sinks = LIST_INIT;

void log_sink_add(log_sink_t *sink) {
    spinlock_acquire(&g_lock);
    list_append(&g_sinks, &sink->list_elem);
    spinlock_release(&g_lock);
}

void log_sink_remove(log_sink_t *sink) {
    spinlock_acquire(&g_lock);
    list_delete(&sink->list_elem);
    spinlock_release(&g_lock);
}

void log(log_level_t level, const char *tag, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_list(level, tag, fmt, list);
    va_end(list);
}

void log_list(log_level_t level, const char *tag, const char *fmt, va_list list) {
    va_list local_list;
    spinlock_acquire(&g_lock);
    LIST_FOREACH(&g_sinks, elem) {
        log_sink_t *sink = LIST_CONTAINER_GET(elem, log_sink_t, list_elem);
        if(sink->level > level) continue;
	    va_copy(local_list, list);
        sink->log(level, tag, fmt, local_list);
        va_end(local_list);
    }
    spinlock_release(&g_lock);
}

void log_raw(char c) {
    spinlock_acquire(&g_lock);
    LIST_FOREACH(&g_sinks, elem) {
        LIST_CONTAINER_GET(elem, log_sink_t, list_elem)->log_raw(c);
    }
    spinlock_release(&g_lock);
}

const char *log_level_tostring(log_level_t level) {
    switch(level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}