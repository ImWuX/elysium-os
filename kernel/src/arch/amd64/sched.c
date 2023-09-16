#include "sched.h"
#include <stdint.h>
#include <string.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <lib/slock.h>
#include <lib/kprint.h>
#include <lib/assert.h>
#include <lib/container.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/lapic.h>

#define KERNEL_STACK_SIZE_PG 4
#define ARCH_THREAD(THREAD) (container_of(THREAD, arch_thread_t, common))

typedef struct {
    uintptr_t rsp;
    uintptr_t base;
    uintptr_t size;
} stack_t;

typedef struct arch_thread {
    struct arch_thread *this;
    stack_t kernel_stack;
    thread_t common;
} arch_thread_t;

typedef struct {
    uint64_t r15, r14, r13, r12, rbp, rbx;
    void (* thread_init)(arch_thread_t *prev);
    void (* thread_func)();
    struct {
        uint64_t rbp;
        uint64_t rip;
    } invalid_stack_frame;
} init_stack_t;

static_assert(offsetof(arch_thread_t, kernel_stack) + offsetof(stack_t, rsp) == 8, "Kernel RSP in thread_t changed. Update arch/amd64/sched.asm");

extern arch_thread_t *sched_context_switch(arch_thread_t *this, arch_thread_t *next);

static long g_next_tid = 1;
static int g_sched_vector = 0;

/*
    @warning The prev parameter relies on the fact
    that sched_context_switch takes a thread "this" which
    will stay in RDI throughout the asm routine and will still
    be present upon entry here
*/
static void sched_thread_init(arch_thread_t *prev) {
    sched_thread_drop(&prev->common);

    lapic_timer_oneshot(g_sched_vector, 1'000'000);

    asm volatile("sti");
}

[[noreturn]] static void sched_idle() {
    while(true) asm volatile("hlt");
    ASSERTC(false, "Unreachable!");
}

static void set_current_thread(arch_thread_t *thread) {
    msr_write(MSR_GS_BASE, (uint64_t) thread);
}

static void sched_switch(arch_thread_t *this, arch_thread_t *next) {
    arch_vmm_load_address_space(next->common.address_space);

    next->common.cpu = this->common.cpu;
    set_current_thread(next);
    this->common.cpu = 0;

    tss_set_rsp0(ARCH_CPU(next->common.cpu)->tss, next->kernel_stack.base);

    arch_thread_t *prev = sched_context_switch(this, next);
    sched_thread_drop(&prev->common);
}

static stack_t allocate_init_stack(uint64_t stack_page_size, void (* func)()) {
    pmm_page_t *stack_page = pmm_alloc_pages(stack_page_size, PMM_GENERAL | PMM_AF_ZERO);
    init_stack_t *init_stack = (init_stack_t *) HHDM(stack_page->paddr + stack_page_size * ARCH_PAGE_SIZE - sizeof(init_stack_t));
    init_stack->thread_init = &sched_thread_init;
    init_stack->thread_func = func;
    return (stack_t) {
        .rsp = (uintptr_t) init_stack,
        .base = HHDM(stack_page->paddr + stack_page_size * ARCH_PAGE_SIZE),
        .size = stack_page_size * ARCH_PAGE_SIZE
    };
}

static arch_thread_t *create_thread(vmm_address_space_t *address_space, stack_t kernel_stack) {
    arch_thread_t *thread = heap_alloc(sizeof(arch_thread_t));
    memset(thread, 0, sizeof(arch_thread_t));
    thread->this = thread;
    thread->common.id = g_next_tid++;
    thread->common.state = THREAD_STATE_READY;
    thread->common.address_space = address_space;
    thread->kernel_stack = kernel_stack;
    return thread;
}

static arch_thread_t *create_kernel_thread(void (* func)()) {
    arch_thread_t *thread = create_thread(&g_kernel_address_space, allocate_init_stack(KERNEL_STACK_SIZE_PG, func));
    list_insert_behind(&g_sched_threads_all, &thread->common.list_all);
    return thread;
}

static void sched_entry([[maybe_unused]] interrupt_frame_t *frame) {
    thread_t *current = arch_sched_current_thread();
    ASSERT(current != 0);

    thread_t *next = sched_thread_next();
    if(!next) {
        if(current == current->cpu->idle_thread) goto oneshot;
        next = current->cpu->idle_thread;
    }
    ASSERT(current != next);

    sched_switch(ARCH_THREAD(current), ARCH_THREAD(next));

    oneshot:
    lapic_timer_oneshot(g_sched_vector, 100'000);
}

thread_t *arch_sched_current_thread() {
    arch_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return &thread->common;
}

[[noreturn]] void sched_init_cpu(arch_cpu_t *cpu) {
    arch_thread_t *idle_thread = heap_alloc(sizeof(arch_thread_t));
    idle_thread->this = idle_thread;
    idle_thread->common.id = 0;
    idle_thread->common.state = THREAD_STATE_READY;
    idle_thread->common.address_space = &g_kernel_address_space;
    idle_thread->common.cpu = 0;
    idle_thread->kernel_stack = allocate_init_stack(KERNEL_STACK_SIZE_PG, &sched_idle);
    cpu->common.idle_thread = &idle_thread->common;

    arch_thread_t *dummy_thread = heap_alloc(sizeof(arch_thread_t));
    memset(dummy_thread, 0, sizeof(arch_thread_t));
    dummy_thread->this = dummy_thread;
    dummy_thread->common.state = THREAD_STATE_DESTROY;
    dummy_thread->common.cpu = &cpu->common;
    sched_switch(dummy_thread, idle_thread);
    __builtin_unreachable();
}

static void temptestfunc() {
    while(true) {
        for(int i = 0; i < 100000000; i++);
        // kprintf("%lu, ", arch_sched_current_thread()->id);
    }
}

void sched_init() {
    int sched_vector = interrupt_request(INTERRUPT_PRIORITY_SCHED, &sched_entry);
    ASSERTC(sched_vector >= 0, "Unable to acquire an interrupt vector for the scheduler");
    g_sched_vector = sched_vector;

    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
    sched_thread_schedule(&create_kernel_thread(&temptestfunc)->common);
}