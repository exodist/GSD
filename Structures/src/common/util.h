/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include "error.h"

typedef struct set set;
typedef rstat(traverse_callback)( set *s, size_t idx, void **cb_args );

uint8_t max_bit( uint64_t num );

rstat threaded_traverse( set *s, size_t start, size_t count, traverse_callback w, void **args, size_t threads );

void *threaded_traverse_worker( void *in );

#endif
