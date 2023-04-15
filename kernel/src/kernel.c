#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}