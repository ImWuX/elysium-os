#include "syscall.h"
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/slock.h>
#include <lib/kprint.h>
#include <memory/heap.h>
#include <memory/vmm.h>
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

int64_t syscall_exit(int code) {
    kprintf("syscall :: exit (code: %i%s, tid: %li)\n", code, code == -0xDEAD ? " (MLIBC PANIC)" : "", arch_sched_thread_current()->id);
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
    int r = vmm_map_direct(arch_sched_thread_current()->proc->address_space, addr, (mapsz + ARCH_PAGE_SIZE - 1) / ARCH_PAGE_SIZE * ARCH_PAGE_SIZE, VMM_PROT_USER | VMM_PROT_WRITE, (uintptr_t) g_fb_context.address - g_hhdm_address);
    if(r != 0) return r;
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

int64_t syscall_dbg(uint64_t ch) {
    putchar((int) ch);
    return 0;
}

uintptr_t syscall_vmm_map(uint64_t size) {
    if(size == 0 || size % ARCH_PAGE_SIZE) return 0;
    static uintptr_t address = 0x50000000;
    int r;
    while(true) {
        r = vmm_map_anon(arch_sched_thread_current()->proc->address_space, address, size, VMM_PROT_USER | VMM_PROT_WRITE, false);
        if(r == 0) break;
        address += ARCH_PAGE_SIZE;
    }
    uintptr_t ret_addr = address;
    address += size;
    kprintf("syscall :: vmm_map(%#lx, %#lx)\n", ret_addr, size);
    return ret_addr;
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