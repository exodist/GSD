/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef BALANCE_H
#define BALANCE_H

#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"
#include "structures.h"
#include <stdint.h>

extern const void *RBLD;

int rebalance( dict *d, location *loc );
size_t rebalance_node( node *n, node ***all, size_t *size, size_t count );
int rebalance_insert_list( dict *d, set *st, slot *s, node **all, size_t start, size_t end, size_t ideal );
int rebalance_insert( dict *d, set *st, slot *s, node *n, size_t ideal );

#endif
