
#ifdef DEV_ASSERTS

#include <assert.h>
#define dev_assert( a ) assert( a )

#else

#define dev_assert( a )

#endif
