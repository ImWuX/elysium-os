#include "common.h"
#include <lib/kprint.h>
#include <arch/types.h>
#include <arch/sched.h>
#include <memory/heap.h>
#include <sys/sched.h>
#include <drivers/acpi.h>
#include <drivers/pci.h>
#include <istyx.h>

void common_init(void *rsdp) {
    acpi_initialize(rsdp);

    pci_enumerate(acpi_find_table((uint8_t *) "MCFG"));

    sched_thread_t *istyx_thread = heap_alloc(sizeof(sched_thread_t));
    arch_sched_init_kernel_thread(istyx_thread, istyx_thread_init);
    sched_add(istyx_thread);
}