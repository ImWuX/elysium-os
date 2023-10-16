#pragma once
#include <lib/panic.h>

#define ASSERT(ASSERTION) if(!(ASSERTION)) panic("Assertion \"%s\" failed\n", #ASSERTION);
#define ASSERTC(ASSERTION, COMMENT) if(!(ASSERTION)) panic("Assertion \"%s\" failed (%s)\n", #ASSERTION, COMMENT)
#define ASSERT_UNREACHABLE() panic("Assertion unreachable failed. %s:%i\n", __FILE__, __LINE__); __builtin_unreachable()