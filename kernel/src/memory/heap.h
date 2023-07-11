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
 * @brief Allocate a block of memory in the heap, without an alignment.
 *
 * @param size Size of block to allocate in bytes
 * @return Address of the allocated block
 */
void *heap_alloc(size_t size);

/**
 * @brief Allocate a block of memory in the heap, with an alignment.
 *
 * @param size Size of block to allocate in bytes
 * @param alignment Alignment of the block
 * @return Address of the allocated block
 */
void *heap_alloc_align(size_t size, size_t alignment);

/**
 * @brief Free a block of memory in the heap.
 *
 * @param address Address of the block to free
 */
void heap_free(void* address);

#endif