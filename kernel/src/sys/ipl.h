#pragma once

typedef enum {
    IPL_SCHED,
    IPL_NORMAL,
    IPL_IPC,
    IPL_CRITICAL
} ipl_t;

/**
 * @brief Swap the IPL level
 * @param ipl new ipl
 * @returns old ipl
 */
ipl_t ipl(ipl_t ipl);