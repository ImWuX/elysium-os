#pragma once
#include <stdarg.h>
#include <lib/list.h>
#include <common/spinlock.h>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

typedef struct {
    char *name;
    log_level_t level;
    list_element_t list_elem;

    /**
     * @brief Called on log
     */
    void (* log)(log_level_t level, const char *tag, const char *fmt, va_list args);

    /**
     * @brief Called on raw log
     * @todo More of a hotfix at the moment
     */
    void (* log_raw)(char c);
} log_sink_t;

/**
 * @brief Add a log sink
 */
void log_sink_add(log_sink_t *sink);

/**
 * @brief Remove a log sink
 */
void log_sink_remove(log_sink_t *sink);

/**
 * @brief Emit a log
 */
void log(log_level_t level, const char *tag, const char *fmt, ...);

/**
 * @brief Log but with a va_list
 */
void log_list(log_level_t level, const char *tag, const char *fmt, va_list list);

/**
 * @brief Log a character without context
 */
void log_raw(char c);

/**
 * @brief Convert a log level to a string
 */
const char *log_level_tostring(log_level_t level);