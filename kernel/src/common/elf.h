#pragma once
#include <stdint.h>
#include <lib/auxv.h>
#include <fs/vfs.h>
#include <memory/vmm.h>

/**
 * @brief Load an ELF as a process image
 * @param node ELF file
 * @param as address space
 * @param entry (out) entry address
 * @param interpreter interpreter or null
 * @returns true on error
 */
bool elf_load(vfs_node_t *node, vmm_address_space_t *as, char **interpreter, auxv_t *auxv);