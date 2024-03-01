#pragma once
#include <common/panic.h>

/**
 * @brief Make an assertion & panic on failure
 */
#define ASSERT(ASSERTION) if(!(ASSERTION)) panic("Assertion \"%s\" failed", #ASSERTION);

/**
 * @brief Make an assertion & panic with a comment on failure
 */
#define ASSERT_COMMENT(ASSERTION, COMMENT) if(!(ASSERTION)) panic("Assertion \"%s\" failed (%s)", #ASSERTION, COMMENT)