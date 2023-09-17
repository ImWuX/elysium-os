#pragma once
#include <memory/vmm.h>

/**
 * @brief Fork an address space
 * 
 * @param root Address space to be forked
 * @return New address space
 */
vmm_address_space_t *vmm_fork(vmm_address_space_t *root);