/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "location.h"
#include "structure.h"
#include "error.h"

rstat op_cmp_delete( dict *d, void *key, void *old_val );
rstat op_cmp_update( dict *d, void *key, void *old_val, void *new_val );
rstat op_delete( dict *d, void *key );
rstat op_dereference( dict *d, void *key );
rstat op_get( dict *d, void *key, void **val );
rstat op_insert( dict *d, void *key, void *val );
rstat op_reference( dict *orig, void *okey, dict *dest, void *dkey );
rstat op_set( dict *d, void *key, void *val );
rstat op_update( dict *d, void *key, void *val );

rstat do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

rstat do_deref( dict *d, void *key, location *loc, sref *swap );

#endif


