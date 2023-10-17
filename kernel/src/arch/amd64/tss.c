#include "tss.h"
#include <lib/c/string.h>

void tss_set_rsp0(tss_t *tss, uintptr_t stack_pointer) {
    tss->rsp0_lower = (uint32_t) (uint64_t) stack_pointer;
    tss->rsp0_upper = (uint32_t) ((uint64_t) stack_pointer >> 32);
}