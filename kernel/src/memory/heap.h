#pragma once
#include <stdint.h>
#include <stddef.h>
#include <memory/vmm.h>

/**
 * @brief Initializes the heap
 */
void heap_initialize(vmm_address_space_t *address_space, size_t size);

/**
 * @brief Allocate a block of memory in the heap, without an alignment
 */
void *heap_alloc(size_t size);

/**
 * @brief Allocate a block of memory in the heap, with an alignment
 */
void *heap_alloc_align(size_t size, size_t alignment);

/**
 * @brief Free a block of memory in the heap
 */
void heap_free(void* address);