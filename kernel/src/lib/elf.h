#pragma once
#include <stdint.h>
#include <fs/vfs.h>
#include <memory/vmm.h>

/**
 * @brief Load an ELF as a process image
 *
 * @param node ELF file
 * @param as Address space
 * @param entry (out) Entry address
 * @return true on error
 */
bool elf_load(vfs_node_t *node, vmm_address_space_t *as, uintptr_t *entry);