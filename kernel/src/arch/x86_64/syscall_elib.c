#include <syscall/syscall.h>
#include <common/log.h>
#include <common/spinlock.h>
#include <arch/x86_64/dev/ps2kb.h>
#include <arch/cpu.h>

static bool g_acquired_input = false;

static spinlock_t g_lock = SPINLOCK_INIT;

static bool got_input = false;
static char input_buffer = '\0';

static void elib_input(uint8_t ch) {
    spinlock_acquire(&g_lock);
    input_buffer = ch;
    got_input = true;
    spinlock_release(&g_lock);
}

syscall_return_t syscall_elib_input() {
    syscall_return_t ret = {};

    if(!g_acquired_input) {
        log(LOG_LEVEL_DEBUG, "SYSCALL", "elib_input() ps2 handoff");
        x86_64_ps2kb_set_handler(elib_input);
        g_acquired_input = true;
    }

    spinlock_acquire(&g_lock);

    int input = -1;
    if(got_input) input = input_buffer;
    got_input = false;

    spinlock_release(&g_lock);

    ret.value = input;
    ret.err = 0;
    return ret;
}