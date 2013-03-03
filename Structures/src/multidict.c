#include <pthread.h>
#include <stdio.h>

#include "multidict.h"
#include "structure.h"
#include "alloc.h"
#include "operations.h"
#include "node_list.h"
#include "balance.h"
#include "util.h"

int SUCCESS = 0;
int FAIL = 1;

dict *clone( dict *d, uint8_t reference, size_t threads ) {
    dict *out = NULL;
    rstat ret = do_create( &out, d->set->settings, d->methods );
    if ( !ret.bit.error && out != NULL ) {
        merge_settings s = { MERGE_INSERT, reference };
        ret = do_merge( d, out, s, threads, NULL );
    }

    if ( ret.bit.error && out != NULL ) {
        dict_free( &out );
        out = NULL;
    }

    return out;
}

rstat merge( dict *from, dict *to, merge_settings s, size_t threads ) {
    return do_merge( from, to, s, threads, NULL );
}

rstat do_merge( dict *orig, dict *dest, merge_settings s, size_t threads, const void *null_swap ) {
    epoch *eo = join_epoch( orig );
    epoch *ed = join_epoch( dest );

    const void *args[4] = { orig, dest, &s, null_swap };

    set *st = orig->set;
    rstat out = threaded_traverse(
        st,
        0, st->settings.slot_count,
        merge_transfer_slot, (void **)args,
        threads
    );

    leave_epoch( orig, eo );
    leave_epoch( dest, ed );
    return out;
}

rstat merge_transfer_slot( set *oset, size_t idx, void **args ) {
    dict *orig = args[0];
    dict *dest = args[1];
    merge_settings *settings = args[2];
    const void *null_swap = args[3];

    rstat out = rstat_ok;
    slot *sl = oset->slots[idx];
    if ( sl == NULL ) return out;

    nlist *nodes = nlist_create();
    if ( !nodes ) return rstat_mem;

    set_spec orig_spec = { 0, 0, NULL, NULL };
    set_spec dest_spec = { 0, 0, NULL, NULL };
    switch ( settings->operation ) {
        case MERGE_SET:
            dest_spec.insert = 1;
            dest_spec.update = 1;
        break;

        case MERGE_INSERT:
            dest_spec.insert = 1;
        break;

        case MERGE_UPDATE:
            dest_spec.update = 1;
        break;
    };

    node *n = sl->root;
    while( n != NULL ) {
        if ( null_swap ) {
            __sync_bool_compare_and_swap( &(n->right), NULL, null_swap );
            __sync_bool_compare_and_swap( &(n->left),  NULL, null_swap );
        }

        if ( n->right && !blocked_null( n->right )) {
            out = nlist_push( nodes, n->right );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        if ( n->left && !blocked_null( n->left )) {
            out = nlist_push( nodes, n->left );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        // If this node has no sref we skip it.
        usref *ur = n->usref;
        if ( null_swap ) __sync_bool_compare_and_swap( &(ur->sref), NULL, null_swap );
        sref *sr = ur->sref;
        xtrn *x  = (sr && !blocked_null( sr )) ? sr->xtrn : NULL;

        if ( x && !blocked_null( x )) {
            if ( settings->reference == 2 ) {
                location *l = NULL;
                set_spec spec = { 1, 0, NULL, ur };
                out = do_set( dest, &l, n->key->value, NULL, &spec );
                free_location( dest, l );
                if ( out.bit.error ) goto MERGE_XFER_ERROR;
            }
            else if ( settings->reference ) {
                out = op_reference( orig, n->key->value, &orig_spec, dest, n->key->value, &dest_spec );
                // Failure is not an error, it means the set_specs prevent us
                // from merging this value, which is desired.
                if ( out.bit.error ) goto MERGE_XFER_ERROR;
            }
            else {
                location *loc = NULL;
                out = do_set( dest, &loc, n->key->value, x->value, &dest_spec );
                if ( loc != NULL ) free_location( dest, loc );
                // Failure is not an error, it means the set_specs prevent us
                // from merging this value, which is desired.
                if ( out.bit.error ) goto MERGE_XFER_ERROR;
            }
        }

        n = nlist_shift( nodes );
    }

    MERGE_XFER_ERROR:
    nlist_free( &nodes );
    return out;
}

rstat do_null_swap( set *s, size_t start, size_t count, const void *from, const void *to, size_t threads ) {
    const void *args[2] = { from, to };
    return threaded_traverse( s, start, count, null_swap_slot, (void **)args, threads );
}

rstat null_swap_slot( set *set, size_t idx, void **args ) {
    const void *from = args[0];
    const void *to   = args[1];

    rstat out = rstat_ok;
    slot *sl = set->slots[idx];
    if ( sl == NULL ) return out;

    nlist *nodes = nlist_create();
    if ( !nodes ) return rstat_mem;

    node *n = sl->root;
    while( n != NULL ) {
        __sync_bool_compare_and_swap( &(n->right), from, to );
        if ( n->right && !blocked_null( n->right )) {
            out = nlist_push( nodes, n->right );
            if ( out.bit.error ) goto NULL_SWAP_ERROR;
        }

        __sync_bool_compare_and_swap( &(n->left), from, to );
        if ( n->left && !blocked_null( n->left )) {
            out = nlist_push( nodes, n->left );
            if ( out.bit.error ) goto NULL_SWAP_ERROR;
        }

        __sync_bool_compare_and_swap( &(n->usref->sref), from, to );

        n = nlist_shift( nodes );
    }

    NULL_SWAP_ERROR:
    nlist_free( &nodes );
    return out;
}

dict *clone_immutable( dict *d, size_t threads ) {
    dict *c = clone( d, 0, threads );
    if ( c == NULL ) return NULL;
    rebalance_all( c, 2, threads );
    c->set->immutable = 1;
    return c;
}
