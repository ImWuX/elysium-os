#include "sched.h"
#include <lib/container.h>
#include <lib/mem.h>
#include <common/assert.h>
#include <memory/heap.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>
#include <sched/sched.h>
#include <sched/thread.h>
#include <arch/types.h>
#include <arch/sched.h>
#include <arch/vmm.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/lapic.h>

#define INTERVAL 100'000
#define KERNEL_STACK_SIZE_PG 4
#define USER_STACK_SIZE (8 * ARCH_PAGE_SIZE)

#define X86_64_THREAD(THREAD) (CONTAINER_OF((THREAD), x86_64_thread_t, common))

typedef struct {
    uintptr_t base;
    uintptr_t size;
} stack_t;

typedef struct x86_64_thread {
    struct x86_64_thread *this;
    uintptr_t rsp;
    stack_t kernel_stack;
    thread_t common;
} x86_64_thread_t;

typedef struct {
    uint64_t r15, r14, r13, r12, rbp, rbx;
    void (* thread_init)(x86_64_thread_t *prev);
    void (* thread_init_kernel)();
    void (* entry)();
    struct {
        uint64_t rbp;
        uint64_t rip;
    } invalid_stack_frame;
} __attribute__((packed)) init_stack_kernel_t;

static_assert(offsetof(x86_64_thread_t, rsp) == 8, "rsp in thread_t changed. Update arch/x86_64/sched.asm::THREAD_RSP_OFFSET");
static_assert(offsetof(x86_64_thread_t, kernel_stack) + offsetof(stack_t, base) == 16, "kernel_stack::base in thread_t changed. Update arch/x86_64/syscall.asm::KERNEL_STACK_BASE_OFFSET");

extern x86_64_thread_t *x86_64_sched_context_switch(x86_64_thread_t *this, x86_64_thread_t *next);

static long g_next_tid = 1;
static int g_sched_vector = 0;

/**
    @warning The prev parameter relies on the fact
    that sched_context_switch takes a thread "this" which
    will stay in RDI throughout the asm routine and will still
    be present upon entry here
*/
static void common_thread_init(x86_64_thread_t *prev) {
    sched_thread_drop(&prev->common);

    x86_64_lapic_timer_oneshot(g_sched_vector, INTERVAL);
}

static void kernel_thread_init() {
    asm volatile("sti");
}

[[noreturn]] static void sched_idle() {
    while(true) asm volatile("hlt");
    ASSERT_COMMENT(false, "Unreachable!");
    __builtin_unreachable();
}

static void sched_switch(x86_64_thread_t *this, x86_64_thread_t *next) {
    if(next->common.proc) {
        arch_vmm_load_address_space(next->common.proc->address_space);
    } else {
        arch_vmm_load_address_space(g_vmm_kernel_address_space);
    }

    next->common.cpu = this->common.cpu;
    x86_64_msr_write(X86_64_MSR_GS_BASE, (uint64_t) next);
    this->common.cpu = 0;

    // tss_set_rsp0(ARCH_CPU(next->common.cpu)->tss, next->kernel_stack.base);

    // if(this->fpu_area) g_fpu_save(this->fpu_area);
    // g_fpu_restore(next->fpu_area);

    x86_64_thread_t *prev = x86_64_sched_context_switch(this, next);
    sched_thread_drop(&prev->common);
}

/** @warning Assumes you have already acquired the lock */
static void destroy_process(process_t *proc) {
    heap_free(proc);
}

/** @warning Thread should not be on the scheduler queue when this is called */
void arch_sched_thread_destroy(thread_t *thread) {
    if(thread->proc) {
        spinlock_acquire(&thread->proc->lock);
        list_delete(&thread->list_proc);
        if(list_is_empty(&thread->proc->threads)) {
            destroy_process(thread->proc);
        } else {
            spinlock_release(&thread->proc->lock);
        }
    }
    heap_free(X86_64_THREAD(thread));
}

static x86_64_thread_t *create_thread(process_t *proc, stack_t kernel_stack, uintptr_t rsp) {
    x86_64_thread_t *thread = heap_alloc(sizeof(x86_64_thread_t));
    memset(thread, 0, sizeof(x86_64_thread_t));
    thread->this = thread;
    thread->common.id = __atomic_fetch_add(&g_next_tid, 1, __ATOMIC_RELAXED);
    thread->common.state = THREAD_STATE_READY;
    thread->common.proc = proc;
    thread->rsp = rsp;
    thread->kernel_stack = kernel_stack;
    // thread->fpu_area = heap_alloc_align(g_fpu_area_size, 64);
    // memset(thread->fpu_area, 0, g_fpu_area_size);

    // TODO: follow sysv abi for how floating points registers should be initialized
    //          (either here, or in the userspace create thread, though I dont see why it would hurt to do this for kernel threads too)
    return thread;
}

thread_t *arch_sched_thread_create_kernel(void (* func)()) {
    pmm_page_t *kernel_stack_page = pmm_alloc_pages(KERNEL_STACK_SIZE_PG, PMM_STANDARD | PMM_FLAG_ZERO);
    stack_t kernel_stack = {
        .base = HHDM(kernel_stack_page->paddr + KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE),
        .size = KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE
    };

    init_stack_kernel_t *init_stack = (init_stack_kernel_t *) (kernel_stack.base - sizeof(init_stack_kernel_t));
    init_stack->entry = func;
    init_stack->thread_init = common_thread_init;
    init_stack->thread_init_kernel = kernel_thread_init;
    return &create_thread(NULL, kernel_stack, (uintptr_t) init_stack)->common;
}


thread_t *arch_sched_thread_current() {
    x86_64_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return &thread->common;
}

void sched_next() {
    thread_t *current = arch_sched_thread_current();
    ASSERT(current != NULL);

    thread_t *next = sched_thread_next();
    if(!next) {
        if(current == current->cpu->idle_thread) goto oneshot;
        next = current->cpu->idle_thread;
    }
    ASSERT(current != next);

    sched_switch(X86_64_THREAD(current), X86_64_THREAD(next));

    oneshot:
    x86_64_lapic_timer_oneshot(g_sched_vector, INTERVAL);
}

static void sched_entry([[maybe_unused]] x86_64_interrupt_frame_t *frame) {
    sched_next();
}

[[noreturn]] void x86_64_sched_init_cpu(x86_64_cpu_t *cpu) {
    x86_64_thread_t *idle_thread = X86_64_THREAD(arch_sched_thread_create_kernel(sched_idle));
    idle_thread->common.id = 0;
    cpu->common.idle_thread = &idle_thread->common;

    x86_64_thread_t *dummy_thread = heap_alloc(sizeof(x86_64_thread_t));
    memset(dummy_thread, 0, sizeof(x86_64_thread_t));
    dummy_thread->this = dummy_thread;
    dummy_thread->common.state = THREAD_STATE_DESTROY;
    dummy_thread->common.cpu = &cpu->common;
    sched_switch(dummy_thread, idle_thread);
    __builtin_unreachable();
}

void x86_64_sched_init() {
    int sched_vector = x86_64_interrupt_request(X86_64_INTERRUPT_PRIORITY_SCHED, sched_entry);
    ASSERT_COMMENT(sched_vector >= 0, "Unable to acquire an interrupt vector for the scheduler");
    g_sched_vector = sched_vector;
}