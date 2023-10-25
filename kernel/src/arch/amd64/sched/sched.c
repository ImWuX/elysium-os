#include "sched.h"
#include <stdint.h>
#include <lib/auxv.h>
#include <lib/slock.h>
#include <lib/kprint.h>
#include <lib/assert.h>
#include <lib/round.h>
#include <lib/container.h>
#include <lib/c/string.h>
#include <sched/sched.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/lapic.h>

#define KERNEL_STACK_SIZE_PG 4
#define USER_STACK_SIZE (8 * ARCH_PAGE_SIZE)
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
    void *fpu_area;
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
} __attribute__((packed)) init_stack_kernel_t;

typedef struct {
    uint64_t r15, r14, r13, r12, rbp, rbx;
    void (* thread_init)(arch_thread_t *prev);
    void (* thread_init_user)();
    void (* entry)();
    uint64_t user_stack;
} __attribute__((packed)) init_stack_user_t;

static_assert(offsetof(arch_thread_t, rsp) == 8, "rsp in thread_t changed. Update arch/amd64/sched/sched.asm::THREAD_RSP_OFFSET");
static_assert(offsetof(arch_thread_t, syscall_rsp) == 16, "syscall_rsp in thread_t changed. Update arch/amd64/sched/syscall.asm::SYSCALL_RSP_OFFSET");
static_assert(offsetof(arch_thread_t, kernel_stack) + offsetof(stack_t, base) == 24, "kernel_stack::base in thread_t changed. Update arch/amd64/sched/syscall.asm::KERNEL_STACK_BASE_OFFSET");

extern arch_thread_t *sched_context_switch(arch_thread_t *this, arch_thread_t *next);
extern void sched_userspace_init();

/* TODO: FPU values originate from init.c (This behavior is going to change) */
extern uint32_t g_fpu_area_size;
extern void (* g_fpu_save)(void *area);
extern void (* g_fpu_restore)(void *area);

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

static void sched_switch(arch_thread_t *this, arch_thread_t *next) {
    if(next->common.proc) {
        arch_vmm_load_address_space(next->common.proc->address_space);
    } else {
        arch_vmm_load_address_space(g_kernel_address_space);
    }

    next->common.cpu = this->common.cpu;
    msr_write(MSR_GS_BASE, (uint64_t) next);
    this->common.cpu = 0;

    tss_set_rsp0(ARCH_CPU(next->common.cpu)->tss, next->kernel_stack.base);

    if(this->fpu_area) g_fpu_save(this->fpu_area);
    g_fpu_restore(next->fpu_area);

    arch_thread_t *prev = sched_context_switch(this, next);
    sched_thread_drop(&prev->common);
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
    thread->fpu_area = heap_alloc_align(g_fpu_area_size, 64);
    memset(thread->fpu_area, 0, g_fpu_area_size);
    // TODO: follow sysv abi for how floating points registers should be initialized
    //          (either here, or in the userspace create thread, though I dont see why it would hurt to do this for kernel threads too)
    return thread;
}

/** @warning Thread should not be on the scheduler queue when this is called */
void arch_sched_thread_destroy(thread_t *thread) {
    slock_acquire(&g_sched_threads_all_lock);
    list_delete(&thread->list_all);
    slock_release(&g_sched_threads_all_lock);
    if(thread->proc) {
        slock_acquire(&thread->proc->lock);
        list_delete(&thread->list_proc);
        if(list_is_empty(&thread->proc->threads)) {
            destroy_process(thread->proc);
        } else {
            slock_release(&thread->proc->lock);
        }
    }
    heap_free(ARCH_THREAD(thread));
}

thread_t *arch_sched_thread_create_kernel(void (* func)()) {
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
    return &thread->common;
}

thread_t *arch_sched_thread_create_user(process_t *proc, uintptr_t ip, uintptr_t sp) {
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
    return &thread->common;
}

uintptr_t arch_sched_stack_setup(process_t *proc, char **argv, char **envp, auxv_t *auxv) {
    #define WRITE_QWORD(VALUE)                                                                                  \
        do {                                                                                                    \
            stack -= sizeof(uint64_t);                                                                          \
            uintptr_t phys_page = arch_vmm_physical(proc->address_space, ROUND_DOWN(stack, ARCH_PAGE_SIZE));    \
            *(volatile uint64_t *) (HHDM(phys_page) + stack % ARCH_PAGE_SIZE) = (VALUE);                        \
        } while(false)
    #define WRITE_STRING(DEST, VALUE, LENGTH)                                                                           \
        do {                                                                                                            \
            for(size_t x = 0; x < (LENGTH); x++) {                                                                      \
                uintptr_t phys_page = arch_vmm_physical(proc->address_space, ROUND_DOWN((DEST), ARCH_PAGE_SIZE));       \
                *(volatile uint8_t *) (HHDM(phys_page) + ((DEST) + x) % ARCH_PAGE_SIZE) = (((uint8_t *) (VALUE))[x]);   \
            };                                                                                                          \
        } while(false)

    uintptr_t stack = arch_vmm_highest_userspace_addr();
    ASSERT(vmm_map_anon(proc->address_space, ROUND_UP(stack, ARCH_PAGE_SIZE) - USER_STACK_SIZE, USER_STACK_SIZE, VMM_PROT_WRITE | VMM_PROT_USER, false) == 0);
    stack &= ~0xF;

    int argc = 0;
    for(; argv[argc]; argc++) stack -= strlen(argv[argc]) + 1;
    uintptr_t arg_data = stack;

    int envc = 0;
    for(; envp[envc]; envc++) stack -= strlen(envp[envc]) + 1;
    uintptr_t env_data = stack;

    stack -= (stack - (12 + 1 + envc + 1 + argc + 1) * sizeof(uint64_t)) % 0x10;

    #define WRITE_AUX(ID, VALUE) WRITE_QWORD(VALUE); WRITE_QWORD(ID);
    WRITE_AUX(0, 0);
    WRITE_AUX(AUXV_SECURE, 0);
    WRITE_AUX(AUXV_ENTRY, auxv->entry);
    WRITE_AUX(AUXV_PHDR, auxv->phdr);
    WRITE_AUX(AUXV_PHENT, auxv->phent);
    WRITE_AUX(AUXV_PHNUM, auxv->phnum);
    #undef WRITE_AUX

    WRITE_QWORD(0);
    for(int i = 0; i < envc; i++) {
        WRITE_QWORD(env_data);
        size_t str_sz = strlen(envp[i]) + 1;
        WRITE_STRING(env_data, envp[i], str_sz);
        env_data += str_sz;
    }

    WRITE_QWORD(0);
    for(int i = 0; i < argc; i++) {
        WRITE_QWORD(arg_data);
        size_t str_sz = strlen(argv[i]) + 1;
        WRITE_STRING(arg_data, argv[i], str_sz);
        arg_data += str_sz;
    }
    WRITE_QWORD(argc);

    return stack;
    #undef WRITE_QWORD
    #undef WRITE_STRING
}

thread_t *arch_sched_thread_current() {
    arch_thread_t *thread = 0;
    asm volatile("mov %%gs:0, %0" : "=r" (thread));
    return &thread->common;
}

void sched_next() {
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

static void sched_entry([[maybe_unused]] interrupt_frame_t *frame) {
    sched_next();
}

[[noreturn]] void sched_init_cpu(arch_cpu_t *cpu) {
    arch_thread_t *idle_thread = ARCH_THREAD(arch_sched_thread_create_kernel(&sched_idle));
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