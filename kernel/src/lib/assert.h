#ifndef LIB_ASSERT_H
#define LIB_ASSERT_H

#include <lib/panic.h>

#define ASSERT(ASSERTION) if(!(ASSERTION)) panic("Assertion \"%s\" failed\n", #ASSERTION);
#define ASSERTC(ASSERTION, COMMENT) if(!(ASSERTION)) panic("Assertion \"%s\" failed (%s)\n", #ASSERTION, COMMENT)

#endif