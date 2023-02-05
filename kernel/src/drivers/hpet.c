#include "hpet.h"
#include <stdbool.h>
#include <memory/hhdm.h>
#include <cpu/irq.h>

#define FEMPTO 1000000000000000

#define GCIDR 0
#define GCR 2
#define MCR 0x1E
#define CCR(n) (0x20 + 4 * n)
#define CVR(n) (0x21 + 4 * n)

#define GCR_ENABLED 1 << 0
#define CCR_INTERRUPTS_ENABLED 1 << 2
#define CCR_PERIODIC 1 << 3
#define CCR_PERIODIC_CAPABLE 1 << 4
#define CCR_64BIT_CAPABLE 1 << 5
#define CCR_TIMER_VALUE_SET 1 << 6

static volatile uint64_t *g_hpet;
static uint16_t g_min_timer_length;
static uint32_t g_freq;

static bool configure_timer(int timer, int irq, int ms, bool one_shot) {
    // printf("CCR: %x\n", g_hpet[CCR(timer)]);
    uint32_t irq_bitmask = g_hpet[CCR(timer)] >> 32;
    if((irq_bitmask & (1 << irq)) == 0) return true;
    if(!one_shot && (g_hpet[CCR(timer)] & CCR_PERIODIC_CAPABLE) == 0) return true;

    g_hpet[CCR(timer)] = (irq & 0x1F) << 9;
    if(!one_shot) {
        g_hpet[CCR(timer)] |= CCR_PERIODIC | CCR_TIMER_VALUE_SET;
    }

    uint64_t ticks = g_freq * ms;
    uint64_t mcr = g_hpet[MCR];
    // printf("MCR:    %i\n", mcr);
    // printf("TICKS:  %i\n", ticks);
    // printf("FIRST:  %i\n", mcr + ticks);
    g_hpet[CVR(timer)] = mcr + ticks;
    if(!one_shot) g_hpet[CVR(timer)] = ticks;

    g_hpet[CCR(timer)] |= CCR_INTERRUPTS_ENABLED;

    return false;
}

static void timer_callback(irq_cpu_register_t registers __attribute__((unused))) {
    // printf("[TIMER] %i\n", g_hpet[MCR]);
}

void hpet_initialize(acpi_sdt_header_t *header) {
    hpet_header_t *hpet_header = (hpet_header_t *) header;
    g_hpet = (uint64_t *) HHDM(hpet_header->address.address);
    g_min_timer_length = hpet_header->minimum_tick;
    g_freq = FEMPTO / (g_hpet[GCIDR] >> 32) / 1000;

    // printf("GCIDR: %x\n", g_hpet[GCIDR]);

    g_hpet[GCR] = 0;
    g_hpet[MCR] = 0;
    g_hpet[GCR] = GCR_ENABLED;

    irq_register_handler(40, timer_callback);
    if(configure_timer(0, 8, 5000, false)) {
        printf("timer errorrrr\n");
    }
}