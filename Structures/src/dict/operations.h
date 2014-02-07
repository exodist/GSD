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

typedef struct set_spec set_spec;
typedef enum create_type create_type;

struct set_spec {
    uint8_t  insert;
    uint8_t  update;
    void    *swap_from;
    usref   *usref;

    dict_trigger *trigger;
    void         *trigger_arg;
};

enum create_type {
    CREATE_XTRN,
    CREATE_SREF,
    CREATE_NODE,
    CREATE_SLOT
};

rstat op_cmp_delete( dict *d, void *key, void *old_val );
rstat op_cmp_update( dict *d, void *key, void *old_val, void *new_val );
rstat op_delete( dict *d, void *key );
rstat op_dereference( dict *d, void *key );
rstat op_get( dict *d, void *key, void **val );
rstat op_insert( dict *d, void *key, void *val );
rstat op_reference( dict *orig, void *okey, set_spec *osp, dict *dest, void *dkey, set_spec *dsp );
rstat op_set( dict *d, void *key, void *val );
rstat op_update( dict *d, void *key, void *val );

rstat op_trigger( dict *d, void *key, dict_trigger *t, void *targ, void *val );

rstat do_deref( dict *d, void *key, location *loc, sref *swap );

rstat do_set( dict *d, location **locator, void *key, void *val, set_spec *spec );

int do_set_sref(   dict *d, location *loc, void *key, void *val, set_spec *spec, void **old_val, rstat *stat );
int do_set_usref(  dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat );
int do_set_parent( dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat );
int do_set_slot(   dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat );

// TODO: is 'eid' necessary?
void *do_set_create( dict *d, uint8_t eid, void *key, void *val, create_type type, set_spec *spec );

#endif


