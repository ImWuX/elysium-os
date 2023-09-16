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
#define USER_STACK_SIZE_PG 8
#define ARCH_THREAD(THREAD) (container_of(THREAD, arch_thread_t, common))

typedef struct {
    uintptr_t base;
    uintptr_t size;
} stack_t;

typedef struct arch_thread {
    struct arch_thread *this;
    uintptr_t rsp;
    stack_t kernel_stack;
    stack_t user_stack;
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
    struct {
        uint64_t rbp;
        uint64_t rip;
    } invalid_stack_frame;
} init_stack_user_t;

static_assert(offsetof(arch_thread_t, rsp) == 8, "Kernel RSP in thread_t changed. Update arch/amd64/sched.asm");

extern arch_thread_t *sched_context_switch(arch_thread_t *this, arch_thread_t *next);
extern void sched_userspace_init();

static long g_next_tid = 1;
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

static void destroy_thread(arch_thread_t *thread) {
    slock_acquire(&g_sched_threads_all_lock);
    list_delete(&thread->common.list_all);
    slock_release(&g_sched_threads_all_lock);
    if(thread->common.proc) {
        slock_acquire(&thread->common.proc->lock);
        list_delete(&thread->common.list_proc);
        slock_release(&thread->common.proc->lock);
    }
    heap_free(thread);
}

static arch_thread_t *create_thread(process_t *proc, stack_t kernel_stack, stack_t user_stack, uintptr_t rsp) {
    arch_thread_t *thread = heap_alloc(sizeof(arch_thread_t));
    memset(thread, 0, sizeof(arch_thread_t));
    thread->this = thread;
    thread->common.id = g_next_tid++;
    thread->common.state = THREAD_STATE_READY;
    thread->common.proc = proc;
    thread->rsp = rsp;
    thread->kernel_stack = kernel_stack;
    thread->user_stack = user_stack;
    return thread;
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

    arch_thread_t *thread = create_thread(0, kernel_stack, (stack_t) {}, (uintptr_t) init_stack);
    slock_acquire(&g_sched_threads_all_lock);
    list_insert_behind(&g_sched_threads_all, &thread->common.list_all);
    slock_release(&g_sched_threads_all_lock);
    return thread;
}

void arch_sched_thread_destroy(thread_t *thread) {
    destroy_thread(ARCH_THREAD(thread));
}

thread_t *arch_sched_thread_create_kernel(void (* func)()) {
    return &create_kernel_thread(func)->common;
}

// static arch_thread_t *create_user_thread() { // TODO: This is very much a test for userspace threads at this point
//     vmm_address_space_t *as = heap_alloc(sizeof(vmm_address_space_t));
//     pmm_page_t *pml4 = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);
//     for(int i = 256; i < 512; i++) {
//         ((uint64_t *) HHDM(pml4->paddr))[i] = ((uint64_t *) HHDM(g_kernel_address_space.archdep.cr3))[i];
//     }
//     as->archdep.cr3 = pml4->paddr;
//     as->lock = SLOCK_INIT;
//     as->segments = LIST_INIT;

//     pmm_page_t *kernel_stack_page = pmm_alloc_pages(KERNEL_STACK_SIZE_PG, PMM_GENERAL | PMM_AF_ZERO);
//     stack_t kernel_stack = {
//         .base = HHDM(kernel_stack_page->paddr + KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE),
//         .size = KERNEL_STACK_SIZE_PG * ARCH_PAGE_SIZE
//     };

//     pmm_page_t *user_stack_page = pmm_alloc_pages(USER_STACK_SIZE_PG, PMM_GENERAL | PMM_AF_ZERO);
//     stack_t user_stack = {
//         .base = ARCH_PAGE_SIZE * (USER_STACK_SIZE_PG + 1),
//         .size = USER_STACK_SIZE_PG * ARCH_PAGE_SIZE
//     };
//     for(int i = 0; i < USER_STACK_SIZE_PG; i++) {
//         arch_vmm_map(as, ARCH_PAGE_SIZE * (i + 1), user_stack_page->paddr + i * ARCH_PAGE_SIZE, VMM_FLAGS_WRITE | VMM_FLAGS_USER);
//     }

//     pmm_page_t *program_page = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);
//     arch_vmm_map(as, 0, program_page->paddr, VMM_FLAGS_EXEC | VMM_FLAGS_USER | VMM_FLAGS_WRITE);
//     *(uint32_t *) (HHDM(program_page->paddr)) = 0xFEEB;

//     init_stack_user_t *init_stack = (init_stack_user_t *) HHDM(user_stack_page->paddr + USER_STACK_SIZE_PG * ARCH_PAGE_SIZE - sizeof(init_stack_user_t));
//     init_stack->entry = 0;
//     init_stack->thread_init = &common_thread_init;
//     init_stack->thread_init_user = &sched_userspace_init;

//     arch_thread_t *thread = create_thread(as, kernel_stack, user_stack, user_stack.base - sizeof(init_stack_user_t));
//     list_insert_behind(&g_sched_threads_all, &thread->common.list_all);
//     return thread;
// }

static void sched_entry([[maybe_unused]] interrupt_frame_t *frame) {
    thread_t *current = arch_sched_thread_current();
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