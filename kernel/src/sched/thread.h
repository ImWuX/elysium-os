#pragma once
#include <lib/list.h>
#include <sys/cpu.h>
#include <memory/vmm.h>

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_ACTIVE,
    THREAD_STATE_DESTROY
} thread_state_t;

typedef struct thread {
    long id;
    thread_state_t state;
    struct cpu *cpu;
    vmm_address_space_t *address_space;
    list_t list_sched;
    list_t list_all;
} thread_t;