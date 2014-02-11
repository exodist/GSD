#ifndef COMMON_DEVTOOLS_H
#define COMMON_DEVTOOLS_H

#ifdef DEV_ASSERTS
#include <assert.h>
#define dev_assert( a ) assert( a )
#define dev_assert_or_do( a ) assert( a )
#else
#define dev_assert( a )
#define dev_assert_or_do( a ) a
#endif


#endif
