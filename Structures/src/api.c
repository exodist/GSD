#include "include/gsd_dict.h"

#include "alloc.h"
#include "dot.h"
#include "merge.h"
#include "operations.h"
#include "settings.h"
#include "structure.h"

int dict_create( dict **d, uint8_t epoch_limit, dict_settings *settings, dict_methods *methods ) {
    return do_create( d, epoch_limit, settings, methods );
}

// Copying and cloning
int merge( dict *from, dict *to );
int merge_refs( dict *from, dict *to );

// Used to free a dict structure.
// Does not free your keys, values, meta, or methods.
int dict_free( dict **d ) {
    return do_free( d );
}

// Used to access your settings
dict_settings *dict_get_settings( dict *d ) {
    return get_settings( d );
}

// Used to get your methods item.
dict_methods *dict_get_methods( dict *d ) {
    return get_methods( d );
}

// -- Informative --

char *dict_dump_dot( dict *d, dict_dot *decode ) {
    return dump_dot( d, decode );
}

// -- Operation --

// This allows you to rebuild your dictionary using new metadata and/or slot
// count. This is useful if you get a DICT_PATHO_ERROR which means the data in
// your dictionary appears to be pathalogical
int dict_reconfigure( dict *d, dict_settings *settings ) {
    return reconfigure( d, settings );
}

// Get never blocks
// Set will insert or update as necessary
// Insert may block in a rebuild or rebalance, fails if key is present
// (DICT_EXIST_ERROR)
// Update never blocks, but fails if the key is not present (DICT_EXIST_ERROR)
// delete removes the value for a key, key will not show in iterations
int dict_get( dict *d, void *key, void **val ) {
    return op_get( d, key, val );
}
int dict_set( dict *d, void *key, void *val ) {
    return op_set( d, key, val );
}
int dict_insert( dict *d, void *key, void *val ) {
    return op_insert( d, key, val );
}
int dict_update( dict *d, void *key, void *val ) {
    return op_update( d, key, val );
}
int dict_delete( dict *d, void *key ) {
    return op_delete( d, key );
}

// cmp_update is just like update() except that you can tell it to only set the
// new value if the old value is what you specify, useful in a threaded program
// to implement a primitive form of transaction for a specific key.
// cmp_delete is just like delete, but it only removes the value if it is what
// you expect.
int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    return op_cmp_update( d, key, old_val, new_val );
}
int dict_cmp_delete( dict *d, void *key, void *old_val ) {
    return op_cmp_delete( d, key, old_val );
}
//int dict_cmp_dereference( dict *fromd, void *fromk, dict *cmpd, void *cmpk ) {
//    return op_cmp_dereference( fromd, fromk, cmpd, cmpk );
//}

// reference allows you to "tie" a key in one dictionary to a key in another
// dictionary.
// dereference allows you to "untie" the key in a dictionary. It can also be
// used on any key to fully remove it from the dictionary, which is something
// delete does not do.
int dict_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    return op_reference( orig, okey, dest, dkey );
}
int dict_dereference( dict *d, void *key ) {
    return op_dereference( d, key );
}

// handler can return false to break loop
int dict_iterate( dict *d, dict_handler *h, void *args ) {
    return dict_iterate( d, h, args );
}

