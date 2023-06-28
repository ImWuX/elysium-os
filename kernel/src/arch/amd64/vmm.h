#ifndef ARCH_AMD64_VMM_H
#define ARCH_AMD64_VMM_H

#include <memory/vmm.h>

/**
 * @brief Create the initial kernel address space.
 * 
 * @param out Kernel address space
 */
void vmm_create_kernel_address_space(vmm_address_space_t *out);

#endif