
#include "include/gsd_dict.h"

#include "devtools.h"
#include "alloc.h"
#include "dot.h"
#include "multidict.h"
#include "operations.h"
#include "settings.h"
#include "structure.h"
#include "balance.h"

dict *dict_build( size_t slots, dict_methods m, void *meta ) {
    dict_settings s = { slots, 16, meta };
    dict *out;
    do_create( &out, s, m );
    return out;
}

rstat dict_create( dict **d, dict_settings settings, dict_methods methods ) {
    return do_create( d, settings, methods );
}

dict_stat dict_merge( dict *from, dict *to, merge_settings s, size_t threads ) {
    dev_assert( s.reference == 0 || s.reference == 1 );
    return merge( from, to, s, threads );
}

dict *dict_clone( dict *d, uint8_t reference, size_t threads ) {
    dev_assert( reference == 0 || reference == 1 );
    return clone( d, reference, threads );
}

rstat dict_free( dict **d ) {
    return do_free( d );
}

dict_settings dict_get_settings( dict *d ) {
    return get_settings( d );
}

dict_methods dict_get_methods( dict *d ) {
    return get_methods( d );
}

char *dict_dump_dot( dict *d, dict_dot *decode ) {
    return dump_dot( d, decode );
}

rstat dict_reconfigure( dict *d, dict_settings settings, size_t max_threads ) {
    return reconfigure( d, settings, max_threads );
}
dict_stat dict_rebalance( dict *d, size_t threads ) {
    return rebalance_all( d, threads );
}

dict_stat dict_insert_trigger( dict *d, void *key, dict_trigger *t, void *targ, void *val ) {
    return op_trigger( d, key, t, targ, val );
}

rstat dict_get( dict *d, void *key, void **val ) {
    return op_get( d, key, val );
}
rstat dict_set( dict *d, void *key, void *val ) {
    return op_set( d, key, val );
}
rstat dict_insert( dict *d, void *key, void *val ) {
    return op_insert( d, key, val );
}
rstat dict_update( dict *d, void *key, void *val ) {
    return op_update( d, key, val );
}
rstat dict_delete( dict *d, void *key ) {
    return op_delete( d, key );
}

rstat dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    return op_cmp_update( d, key, old_val, new_val );
}
rstat dict_cmp_delete( dict *d, void *key, void *old_val ) {
    return op_cmp_delete( d, key, old_val );
}

rstat dict_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    set_spec sp = { 1, 1, NULL, NULL };
    return op_reference( orig, okey, &sp, dest, dkey, &sp );
}
rstat dict_dereference( dict *d, void *key ) {
    return op_dereference( d, key );
}

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    return iterate( d, h, args );
}

dict *dict_clone_immutable( dict *d, size_t threads ) {
    return clone_immutable( d, threads );
}

