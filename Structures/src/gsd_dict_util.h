/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef GSD_DICT_UTIL_H
#define GSD_DICT_UTIL_H

#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"
#include "gsd_dict_structures.h"
#include <stdint.h>

uint8_t max_bit( uint64_t num );

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args );

int dict_do_create( dict **d, uint8_t epoch_limit, dict_settings *settings, dict_methods *methods );

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

int dict_do_deref( dict *d, void *key, location *loc, sref *swap );

set *dict_create_set( dict_settings *settings );

#endif
