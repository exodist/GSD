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

void free_set(  void *ptr, void *arg );
void free_slot( void *ptr, void *arg );
void free_node( void *ptr, void *arg );
void free_sref( void *ptr, void *arg );
void free_xtrn( void *ptr, void *arg );

rstat do_create( dict **d, dict_settings settings, dict_methods methods );

set   *create_set( dict_settings settings, size_t slot_count );
slot  *create_slot( node *root );
node  *create_node( xtrn *key, usref *ref, size_t min_ref_count );
usref *create_usref( sref *ref );
sref  *create_sref( xtrn *x, trigger_ref *t );
xtrn  *create_xtrn( dict *d, void *value );

#endif
