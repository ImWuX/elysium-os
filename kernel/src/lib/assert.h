#pragma once
#include <lib/panic.h>

/**
 * @brief Make an assertion & panic on failure.
 * @param ASSERTION
 */
#define ASSERT(ASSERTION) if(!(ASSERTION)) panic("Assertion \"%s\" failed\n", #ASSERTION);

/**
 * @brief Make an assertion & panic with a comment on failure.
 * @param ASSERTION
 * @param COMMENT
 */
#define ASSERTC(ASSERTION, COMMENT) if(!(ASSERTION)) panic("Assertion \"%s\" failed (%s)\n", #ASSERTION, COMMENT)