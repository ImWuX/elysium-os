#include "syscall.h"
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/slock.h>
#include <lib/kprint.h>
#include <memory/heap.h>
#include <memory/hhdm.h>
#include <sched/sched.h>
#include <arch/sched.h>
#include <arch/vmm.h>
#include <arch/amd64/sched/sched.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <graphics/draw.h>

#define MSR_EFER_SCE 1

extern void syscall_entry();

extern draw_context_t g_fb_context;

static slock_t g_kbq_lock = SLOCK_INIT;
static list_t g_kbq = LIST_INIT_CIRCULAR(g_kbq);

int64_t syscall_exit() {
    arch_sched_thread_current()->state = THREAD_STATE_DESTROY;
    sched_next();
    __builtin_unreachable();
}

int64_t syscall_write(uint64_t ch) {
    putchar((int) ch);
    return 0;
}

typedef struct {
    uint64_t addr;
    uint64_t pitch;
    uint64_t width;
    uint64_t height;
} elib_fb_t;

int64_t syscall_fb(uint64_t addr, elib_fb_t *fb) {
    ASSERTC(arch_sched_thread_current()->proc, "Should be a userspace thread");
    uint64_t mapsz = g_fb_context.pitch * g_fb_context.height * sizeof(uint32_t);
    for(uint64_t i = 0; i < (mapsz + ARCH_PAGE_SIZE - 1) / ARCH_PAGE_SIZE; i++) {
        arch_vmm_map(arch_sched_thread_current()->proc->address_space, addr + i * ARCH_PAGE_SIZE, (uintptr_t) g_fb_context.address - g_hhdm_address + i * ARCH_PAGE_SIZE, VMM_FLAGS_USER | VMM_FLAGS_WRITE);
    }
    fb->addr = addr;
    fb->width = g_fb_context.width;
    fb->height = g_fb_context.height;
    fb->pitch = g_fb_context.pitch;
    return 0;
}

typedef struct {
    uint8_t ch;
    list_t list;
} kb_event_t;

int64_t syscall_kbin(char *ch) {
    slock_acquire(&g_kbq_lock);
    if(list_is_empty(&g_kbq)) {
        slock_release(&g_kbq_lock);
        return -1;
    }
    kb_event_t *ev = LIST_GET(g_kbq.next, kb_event_t, list);
    list_delete(&ev->list);
    *ch = (char) ev->ch;
    heap_free(ev);
    slock_release(&g_kbq_lock);
    return 0;
}

static void consume_kb_event(uint8_t ch) {
    kb_event_t *kbev = heap_alloc(sizeof(kb_event_t));
    kbev->ch = ch;
    slock_acquire(&g_kbq_lock);
    list_insert_before(&g_kbq, &kbev->list);
    slock_release(&g_kbq_lock);
}

void syscall_init() {
    msr_write(MSR_EFER, msr_read(MSR_EFER) | MSR_EFER_SCE);
    msr_write(MSR_STAR, ((uint64_t) GDT_CODE_RING0 << 32) | ((uint64_t) (GDT_CODE_RING3 - 16) << 48));
    msr_write(MSR_LSTAR, (uint64_t) &syscall_entry);
    msr_write(MSR_SFMASK, msr_read(MSR_SFMASK) | (1 << 9));

    ps2kb_set_handler(&consume_kb_event);
}