#pragma once
#include <stdint.h>
#include <fs/vfs.h>
#include <memory/vmm.h>

#define ELF_AUXV_NULL 0
#define ELF_AUXV_IGNORE 1
#define ELF_AUXV_EXECFD 2
#define ELF_AUXV_PHDR 3
#define ELF_AUXV_PHENT 4
#define ELF_AUXV_PHNUM 5
#define ELF_AUXV_PAGESZ 6
#define ELF_AUXV_BASE 7
#define ELF_AUXV_FLAGS 8
#define ELF_AUXV_ENTRY 9
#define ELF_AUXV_NOTELF 10
#define ELF_AUXV_UID 11
#define ELF_AUXV_EUID 12
#define ELF_AUXV_GID 13
#define ELF_AUXV_EGID 14

typedef struct {
    uint64_t entry;
    uint64_t phdr;
    uint64_t phent;
    uint64_t phnum;
} elf_auxv_t;

/**
 * @brief Load an ELF as a process image
 *
 * @param node ELF file
 * @param as Address space
 * @param entry (out) Entry address
 * @param interpreter Interpreter or null
 * @return true on error
 */
bool elf_load(vfs_node_t *node, vmm_address_space_t *as, char **interpreter, elf_auxv_t *auxv);