#pragma once
#include <stdint.h>

#ifdef __ARCH_AMD64

#define ARCH_PAGE_SIZE 0x1000

#define ARCH_USERSPACE_START 0x1000
#define ARCH_HHDM_START 0xFFFF'8000'0000'0000
#define ARCH_HHDM_END 0xFFFF'8400'0000'0000
#define ARCH_KHEAP_START 0xFFFF'8400'0000'0000
#define ARCH_KHEAP_END 0xFFFF'8500'0000'0000

#else
#error Invalid arch
#endif