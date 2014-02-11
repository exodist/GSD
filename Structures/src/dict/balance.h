/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef DICT_BALANCE_H
#define DICT_BALANCE_H

#include <stdint.h>

#include "structure.h"
#include "location.h"
#include "error.h"
#include "magic_pointers.h"

rstat rebalance( dict *d, set *st, size_t slotn, size_t *count_diff );
size_t rebalance_add_node( node *n, node ***all, size_t *size, size_t count );
rstat rebalance_insert_list( dict *d, set *st, slot **s, node **all, size_t start, size_t end, size_t ideal );
rstat rebalance_insert( dict *d, set *st, slot **s, node *n, size_t ideal );

rstat balance_check( dict *d, location *loc, size_t count );

void rebalance_unblock( node *n );

rstat rebalance_all( dict *d, size_t threads );

rstat rebalance_callback( set *s, size_t idx, void **cb_args );

#endif
