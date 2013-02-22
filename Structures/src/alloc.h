/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef ALLOC_H
#define ALLOC_H

#include "structure.h"
#include "error.h"

// Also in include/gsd_dict.h
rstat do_free( dict **dr );

void free_trash( dict *d, trash *t );
void free_set( dict *d, set *s );
void free_slot( dict *d, slot *s );
void free_node( dict *d, node *n );
void free_sref( dict *d, sref *r );
void free_xtrn( dict *d, xtrn *x );

rstat do_create( dict **d, uint8_t epoch_limit, dict_settings *settings, dict_methods *methods );

set   *create_set( dict_settings *settings );
slot  *create_slot( node *root );
node  *create_node( xtrn *key, usref *ref, size_t min_ref_count );
usref *create_usref( sref *ref );
sref  *create_sref( xtrn *x    );
xtrn  *create_xtrn( dict *d, void *value );

#endif
