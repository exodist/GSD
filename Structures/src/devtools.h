#ifndef DEVTOOLS_H
#define DEVTOOLS_H

#ifdef DEV_ASSERTS
#include <assert.h>
#define dev_assert( a ) assert( a )
#else
#define dev_assert( a )
#endif

#endif
