#pragma once

#ifdef __ARCH_X86_64
#define ARCH_PAGE_SIZE 0x1000
#else
#error Unimplemented
#endif

static_assert(ARCH_PAGE_SIZE > 0);