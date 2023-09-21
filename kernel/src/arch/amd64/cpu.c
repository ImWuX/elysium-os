#include "cpu.h"
#include <arch/cpu.h>

void arch_cpu_relax() {
    __builtin_ia32_pause();
}