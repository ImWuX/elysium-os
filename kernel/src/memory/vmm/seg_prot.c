#include <memory/vmm.h>
#include <lib/panic.h>

static void seg_attach(vmm_segment_t *segment) {
    panic("SEG_PROT: Tried attaching a protected segment (%s)\n", (char *) segment->driver_data);
}

static void seg_detach(vmm_segment_t *segment) {
    panic("SEG_PROT: Tried detaching a protected segment (%s)\n", (char *) segment->driver_data);
}

static bool seg_fault([[maybe_unused]] vmm_segment_t *segment, [[maybe_unused]] uintptr_t address, [[maybe_unused]] int flags) {
    return false;
}

seg_driver_t g_seg_prot = (seg_driver_t) {
    .name = "protected",
    .ops = {
        .attach = seg_attach,
        .detach = seg_detach,
        .fault = seg_fault
    }
};