/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef FREE_H
#define FREE_H

#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"
#include "structures.h"

void dict_free_set( dict *d, set *s );
void dict_free_slot( dict *d, void *meta, slot *s );
void dict_free_node( dict *d, void *meta, node *n );
void dict_free_sref( dict *d, void *meta, sref *r );

#endif
