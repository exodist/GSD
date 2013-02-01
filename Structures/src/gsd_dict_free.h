/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the gsd_dict_api.h header file in external programs.
\*/

#ifndef GSD_DICT_FREE_H
#define GSD_DICT_FREE_H

#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"

void dict_free_set( dict *d, set *s );
void dict_free_slot( dict *d, void *meta, slot *s );
void dict_free_node( dict *d, void *meta, node *n );
void dict_free_sref( dict *d, sref *r );

#endif
