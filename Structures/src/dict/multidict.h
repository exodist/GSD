/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef MULTIDICT_H
#define MULTIDICT_H

#include "include/gsd_dict.h"
#include "structure.h"
#include "error.h"

typedef struct dict_merge_settings merge_settings;

dict *clone( dict *d, uint8_t reference, size_t threads );

rstat merge( dict *from, dict *to, merge_settings s, size_t threads );

rstat do_merge( dict *orig, dict *dest, merge_settings s, size_t threads, const void *null_swap );

void *merge_worker( void *args );

rstat merge_transfer_slot( set *oset, size_t idx, void **args );

rstat do_null_swap( set *s, size_t start, size_t count, const void *from, const void *to, size_t threads );

rstat null_swap_slot( set *set, size_t idx, void **args );

dict *clone_immutable( dict *d, size_t threads );

#endif
