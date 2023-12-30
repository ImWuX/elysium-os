#include <tartarus.h>

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    for(;;);
    __builtin_unreachable();
}