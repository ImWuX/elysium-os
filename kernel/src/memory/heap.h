#ifndef MEMORY_HEAP_H
#define MEMORY_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <memory/vmm.h>

/**
 * @brief Initialize the heap.
 * 
 * @param address_space Address space
 * @param start Starting vaddr for the heap
 * @param end Ending vaddr for the heap (Heap will expand up to this point)
 */
void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end);

/**
 * @brief Allocate a block memory in the heap.
 * 
 * @param size Size of block to allocate in bytes
 * @return Address of the allocated memory
 */
void *heap_alloc(size_t size);

/**
 * @brief Free a block of memory in the heap.
 * 
 * @param address Address of the block of memory
 */
void heap_free(void* address);

#endif