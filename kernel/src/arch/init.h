#ifndef ARCH_INIT_H
#define ARCH_INIT_H

#include <tartarus.h>

/**
 * @brief Arch specific initialization
 * 
 * @param boot_params Parameters passed by Tartarus
 */
void arch_init(tartarus_boot_info_t *boot_info);

#endif