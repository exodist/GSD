#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include "resize.h"
#include "node_list.h"
#include "balance.h"
#include "location.h"
#include "alloc.h"
#include "util.h"
#include "dot.h"
#include "include/gsd_dict.h"

int SUCCESS = 0;
int FAIL = 1;

rstat resize( dict *d, size_t slot_count, size_t max_threads ) {
    epoch *e = join_epoch( d );
    set *s = d->set;

    if ( !__sync_bool_compare_and_swap( &(s->rebuild), 0, 1 )) {
        leave_epoch( d, e );
        return rstat_trans;
    }

    dict_settings settings = d->set->settings;
    settings.min_slot_count = slot_count;
    settings.max_slot_count = slot_count;
    settings.max_imbalance = 0;
    dict *new_dict = NULL;
    do_create( &new_dict, d->epoch_limit, settings, d->methods );

    size_t index = 0;
    void *args[4] = { &index, s, d, new_dict };

    size_t tcount = max_threads < s->slot_count ? max_threads : s->slot_count;

    int fail = 0;
    if ( max_threads < 2 ) {
        fail = *(int *)(resize_worker( args ));
    }
    else {
        pthread_t *pts = malloc( tcount * sizeof( pthread_t ));
        for ( int i = 0; i < tcount; i++ ) {
            pthread_create( &(pts[i]), NULL, resize_worker, args );
        }
        for ( int i = 0; i < tcount; i++ ) {
            int *ret;
            pthread_join( pts[i], (void **)&ret );
            fail += *ret;
        }
    }

    epoch *de = join_epoch( new_dict );

    rstat out = rstat_ok;
    if ( fail ) {
        out = make_error( 1, 0, DICT_UNKNOWN, 14, __LINE__, __FILE__ );
    }
    else {
        new_dict->set->settings = s->settings;
        if ( __sync_bool_compare_and_swap( &(d->set), s, new_dict->set )) {
            __sync_add_and_fetch( &(d->resized), 1 );
            dispose( d, (trash *)s );
            rebalance_all( new_dict, 2, tcount );
            new_dict->set = NULL;
        }
        else {
            out = rstat_trans;
        }
    }

    leave_epoch( new_dict, de );
    do_free( &new_dict );
    leave_epoch( d, e );

    return out;
}

void *resize_worker( void *in ) {
    void **args = (void **)in;
    size_t *index = args[0];
    set    *s     = args[1];
    dict   *d     = args[2];
    dict   *nd    = args[3];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= s->slot_count ) return (void *)&SUCCESS;
        if ( !resize_transfer_slot( s, idx, d, nd )) return (void *)&FAIL;
    }
}

int resize_transfer_slot( set *s, size_t idx, dict *orig, dict *dest ) {
    slot *sl = s->slots[idx];

    // We need to lock the slot against further inserts.
    while ( !__sync_bool_compare_and_swap( &(sl->rebuild), 0, 1 )) {
        sleep( 0 );
        sl = s->slots[idx];
    }

    nlist *nodes = nlist_create();
    if ( !nodes ) return 0;

    rstat check = nlist_push( nodes, sl->root );
    if ( check.bit.error ) goto RESIZE_ERROR;

    node *n = nlist_shift( nodes );
    while( n != NULL ) {
        // If right is NULL we block it off with RBLD, otherwise add to list
        __sync_bool_compare_and_swap( &(n->right), NULL, RBLD );
        if ( n->right != RBLD ) {
            check = nlist_push( nodes, n->right );
            if ( check.bit.error ) goto RESIZE_ERROR;
        }
        // If left is NULL we block it off with RBLD, otherwise add to list
        __sync_bool_compare_and_swap( &(n->left), NULL, RBLD );
        if ( n->left != RBLD ) {
            check = nlist_push( nodes, n->left );
            if ( check.bit.error ) goto RESIZE_ERROR;
        }

        // Mark a node with no sref as rebuild
        __sync_bool_compare_and_swap( &(n->usref->sref), NULL, RBLD );

        // If this node has no sref we skip it.
        sref *sr = n->usref->sref;
        if ( sr == RBLD ) {
            n = nlist_shift( nodes );
            continue;
        }

        rstat st = dict_reference( orig, n->key->value, dest, n->key->value );
        if ( !st.bit.fail ) {
            n = nlist_shift( nodes );
            continue;
        }

        goto RESIZE_ERROR;
    }

    nlist_free( &nodes );
    return 1;

    RESIZE_ERROR:
    rebalance_unblock( sl->root );
    nlist_free( &nodes );
    assert( __sync_bool_compare_and_swap( &(sl->rebuild), 1, 0 ) );
    return 0;
}

rstat resize_check( dict *d ) {
    rstat out = rstat_ok;
    size_t count = d->item_count;
    epoch *e = join_epoch( d );

    set *s = d->set;
    if ( s->rebuild ) goto END_RESIZE_CHECK;
    size_t slotc = s->slot_count;

    // Not allowed to grow
    if ( slotc >= s->settings.max_slot_count ) goto END_RESIZE_CHECK;

    size_t ideal_height = max_bit( count / slotc );
    if ( ideal_height <= 16 ) goto END_RESIZE_CHECK;

    while ( ideal_height > 16 ) {
        slotc *= 2;
        ideal_height = max_bit( count / slotc );
    }

    if ( slotc > s->settings.max_slot_count ) slotc = s->settings.max_slot_count;
    out = resize( d, slotc, s->settings.max_internal_threads );

    END_RESIZE_CHECK:
    leave_epoch( d, e );
    return out;
}
