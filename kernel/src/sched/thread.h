#pragma once
#include <lib/list.h>
#include <sched/process.h>
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
    process_t *proc;
    list_t list_sched;
    list_t list_proc;
    list_t list_all;
} thread_t;