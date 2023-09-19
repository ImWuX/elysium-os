#include "sched.h"
#include <stdint.h>
#include <string.h>
#include <lib/slock.h>
#include <lib/kprint.h>
#include <lib/assert.h>
#include <lib/container.h>
#include <sched/sched.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/amd64/vmm.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/lapic.h>

#define KERNEL_STACK_SIZE_PG 4
#define USER_STACK_SIZE_PG 8
#define ARCH_THREAD(THREAD) (container_of(THREAD, arch_thread_t, common))

typedef struct {
    uintptr_t base;
    uintptr_t size;
} stack_t;

typedef struct arch_thread {
    struct arch_thread *this;
    uintptr_t rsp;
    uintptr_t syscall_rsp;
    stack_t kernel_stack;
    thread_t common;
} arch_thread_t;

typedef struct {
    uint64_t r15, r14, r13, r12, rbp, rbx;
    void (* thread_init)(arch_thread_t *prev);
    void (* thread_init_kernel)();
    void (* entry)();
    struct {
        uint64_t rbp;
        uint64_t rip;
    } invalid_stack_frame;
} init_stack_kernel_t;

typedef struct {
    uint64_t r15, r14, r13, r12, rbp, rbx;
    void (* thread_init)(arch_thread_t *prev);
    void (* thread_init_user)();
    void (* entry)();
    uint64_t user_stack;
} init_stack_user_t;

static_assert(offsetof(arch_thread_t, rsp) == 8, "rsp in thread_t changed. Update arch/amd64/sched/sched.asm::THREAD_RSP_OFFSET");
static_assert(offsetof(arch_thread_t, syscall_rsp) == 16, "syscall_rsp in thread_t changed. Update arch/amd64/sched/syscall.asm::SYSCALL_RSP_OFFSET");
static_assert(offsetof(arch_thread_t, kernel_stack) + offsetof(stack_t, base) == 24, "kernel_stack::base in thread_t changed. Update arch/amd64/sched/syscall.asm::KERNEL_STACK_BASE_OFFSET");

extern arch_thread_t *sched_context_switch(arch_thread_t *this, arch_thread_t *next);
extern void sched_userspace_init();

static long g_next_tid = 1;
static long g_next_pid = 1;
static int g_sched_vector = 0;

/*
    @warning The prev parameter relies on the fact
    that sched_context_switch takes a thread "this" which
    will stay in RDI throughout the asm routine and will still
    be present upon entry here
*/
static void common_thread_init(arch_thread_t *prev) {
    sched_thread_drop(&prev->common);

    lapic_timer_oneshot(g_sched_vector, 1'000'000);
}

static void kernel_thread_init() {
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
    if(next->common.proc) {
        arch_vmm_load_address_space(next->common.proc->address_space);
    } else {
        arch_vmm_load_address_space(&g_kernel_address_space);
    }

    next->common.cpu = this->common.cpu;
    set_current_thread(next);
    this->common.cpu = 0;

    tss_set_rsp0(ARCH_CPU(next->common.cpu)->tss, next->kernel_stack.base);

    arch_thread_t *prev = sched_context_switch(this, next);
    sched_thread_drop(&prev->common);
}

static void sched_entry([[maybe_unused]] interrupt_frame_t *frame) {
    thread_t *current = arch_sched_thread_current();
    ASSERT((uintptr_t) current >= g_hhdm_address);

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

static process_t *create_process(vmm_address_space_t *address_space) {
    process_t *proc = heap_alloc(sizeof(process_t));
    proc->id = __atomic_fetch_add(&g_next_pid, 1, __ATOMIC_RELAXED);
    proc->lock = SLOCK_INIT;
    proc->threads = LIST_INIT;
    proc->address_space = address_space;
    return proc;
}

/** @warning Assumes you have already acquired the lock */
static void destroy_process(process_t *proc) {
    heap_free(proc);
}

static arch_thread_t *create_thread(process_t *proc, stack_t kernel_stack, uintptr_t rsp) {
    arch_thread_t *thread = heap_alloc(sizeof(arch_thread_t));
    memset(thread, 0, sizeof(arch_thread_t));
    thread->this = thread;
    thread->common.id = __atomic_fetch_add(&g_next_tid, 1, __ATOMIC_RELAXED);
    thread->common.state = THREAD_STATE_READY;
    thread->common.proc = proc;
    thread->rsp = rsp;
    thread->kernel_stack = kernel_stack;
    return thread;
}

/** @warning Thread should not be on the scheduler queue when this is called */
static void destroy_thread(arch_thread_t *thread) {
    slock_acquire(&g_sched_threads_all_lock);
    list_delete(&thread->common.list_all);
    slock_release(&g_sched_threads_all_lock);
    if(thread->common.proc) {
        slock_acquire(&thread->common.proc->lock);
        list_delete(&thread->common.list_proc);
        if(list_is_empty(&thread->common.proc->threads)) {
            destroy_process(thread->common.proc);
        } else {
            slock_release(&thread->common.proc->lock);
        }
    }
    heap_free(thread);
}

static arch_thread_t *create_kernel_thread(void (* func)()) {
    pmm_page_t *kernel_stack_page = pmm_alloc_pages(KERNEL_STACK_SIZE_PG, PMM_GENERAL | PMM_AF_ZERO);
    stack_t kernel_stack = {
        .base = HHDM(kernel_stack_page->paddr + KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE),
        .size = KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE
    };

    init_stack_kernel_t *init_stack = (init_stack_kernel_t *) (kernel_stack.base - sizeof(init_stack_kernel_t));
    init_stack->entry = func;
    init_stack->thread_init = &common_thread_init;
    init_stack->thread_init_kernel = &kernel_thread_init;

    arch_thread_t *thread = create_thread(0, kernel_stack, (uintptr_t) init_stack);
    slock_acquire(&g_sched_threads_all_lock);
    list_insert_behind(&g_sched_threads_all, &thread->common.list_all);
    slock_release(&g_sched_threads_all_lock);
    return thread;
}

static arch_thread_t *create_user_thread(process_t *proc, uintptr_t ip, uintptr_t sp) { // TODO: This is very much a test for userspace threads at this point
    pmm_page_t *kernel_stack_page = pmm_alloc_pages(KERNEL_STACK_SIZE_PG, PMM_GENERAL | PMM_AF_ZERO);
    stack_t kernel_stack = {
        .base = HHDM(kernel_stack_page->paddr + KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE),
        .size = KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE
    };

    init_stack_user_t *init_stack = (init_stack_user_t *) (kernel_stack.base - sizeof(init_stack_user_t));
    init_stack->entry = (void (*)()) ip;
    init_stack->thread_init = &common_thread_init;
    init_stack->thread_init_user = &sched_userspace_init;
    init_stack->user_stack = sp;

    arch_thread_t *thread = create_thread(proc, kernel_stack, (uintptr_t) init_stack);
    slock_acquire(&g_sched_threads_all_lock);
    list_insert_behind(&g_sched_threads_all, &thread->common.list_all);
    slock_release(&g_sched_threads_all_lock);
    slock_acquire(&proc->lock);
    list_insert_behind(&proc->threads, &thread->common.list_proc);
    slock_release(&proc->lock);
    return thread;
}

void arch_sched_thread_destroy(thread_t *thread) {
    destroy_thread(ARCH_THREAD(thread));
}

process_t *arch_sched_process_create(vmm_address_space_t *address_space) {
    return create_process(address_space);
}

thread_t *arch_sched_thread_create_kernel(void (* func)()) {
    return &create_kernel_thread(func)->common;
}

thread_t *arch_sched_thread_create_user(process_t *proc, uintptr_t ip, uintptr_t sp) {
    return &create_user_thread(proc, ip, sp)->common;
}

thread_t *arch_sched_thread_current() {
    arch_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return &thread->common;
}

[[noreturn]] void sched_init_cpu(arch_cpu_t *cpu) {
    arch_thread_t *idle_thread = create_kernel_thread(&sched_idle);
    idle_thread->common.id = 0;
    cpu->common.idle_thread = &idle_thread->common;

    arch_thread_t *dummy_thread = heap_alloc(sizeof(arch_thread_t));
    memset(dummy_thread, 0, sizeof(arch_thread_t));
    dummy_thread->this = dummy_thread;
    dummy_thread->common.state = THREAD_STATE_DESTROY;
    dummy_thread->common.cpu = &cpu->common;
    sched_switch(dummy_thread, idle_thread);
    __builtin_unreachable();
}

void sched_init() {
    int sched_vector = interrupt_request(INTERRUPT_PRIORITY_SCHED, &sched_entry);
    ASSERTC(sched_vector >= 0, "Unable to acquire an interrupt vector for the scheduler");
    g_sched_vector = sched_vector;
}