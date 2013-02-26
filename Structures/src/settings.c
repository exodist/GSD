#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include "include/gsd_dict.h"

#include "settings.h"
#include "structure.h"
#include "node_list.h"
#include "balance.h"
#include "location.h"
#include "alloc.h"
#include "util.h"

int SUCCESS = 0;
int FAIL = 1;

dict_settings get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods get_methods( dict *d ) {
    return d->methods;
}

rstat reconfigure( dict *d, dict_settings settings, size_t max_threads ) {
    epoch *e = join_epoch( d );
    rstat out = rstat_ok;

    set *s = d->set;
    if ( __sync_bool_compare_and_swap( &(s->rebuild), 0, 1 )) {
        s->settings.max_imbalance = settings.max_imbalance;
        s->settings.max_internal_threads = settings.max_internal_threads;

        size_t count = settings.slot_count;
        void  *meta  = settings.meta;

        if ( count != s->settings.slot_count || meta != s->settings.meta ) {
            out = do_reconfigure( d, count, meta, max_threads );
        }

        __sync_bool_compare_and_swap( &(s->rebuild), 1, 0 );
    }
    else {
        out = rstat_trans;
    }

    leave_epoch( d, e );
    return out;
}

rstat do_reconfigure( dict *d, size_t slot_count, void *meta, size_t max_threads ) {
    set *s = d->set;

    dict_settings settings = d->set->settings;
    settings.max_imbalance = 0;
    if ( slot_count ) settings.slot_count = slot_count;
    if ( meta )       settings.meta       = meta;

    dict *new_dict = NULL;
    do_create( &new_dict, d->epoch_limit, settings, d->methods );

    size_t index = 0;
    void *args[4] = { &index, s, d, new_dict };

    size_t tcount = max_threads < s->settings.slot_count ? max_threads : s->settings.slot_count;

    int fail = 0;
    if ( max_threads < 2 ) {
        fail = *(int *)(reconf_worker( args ));
    }
    else {
        pthread_t *pts = malloc( tcount * sizeof( pthread_t ));
        for ( int i = 0; i < tcount; i++ ) {
            pthread_create( &(pts[i]), NULL, reconf_worker, args );
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
        // Unblock slots
        for ( size_t i = 0; i < s->settings.slot_count; i++ ) {
            slot *sl = s->slots[i];
            if ( sl->rebuild == 2 ) {
                rebalance_unblock( sl->root );
                assert( __sync_bool_compare_and_swap( &(sl->rebuild), 2, 0 ) );
            }
        }

        out = make_error( 1, 0, DICT_UNKNOWN, 14, __LINE__, __FILE__ );
    }
    else {
        new_dict->set->settings = s->settings;
        if ( __sync_bool_compare_and_swap( &(d->set), s, new_dict->set )) {
            d->set->settings.max_imbalance = s->settings.max_imbalance;
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

    return out;
}

void *reconf_worker( void *in ) {
    void **args = (void **)in;
    size_t *index = args[0];
    set    *s     = args[1];
    dict   *d     = args[2];
    dict   *nd    = args[3];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= s->settings.slot_count ) return (void *)&SUCCESS;
        if ( !reconf_transfer_slot( s, idx, d, nd )) return (void *)&FAIL;
    }
}

int reconf_transfer_slot( set *s, size_t idx, dict *orig, dict *dest ) {
    slot *sl = s->slots[idx];

    // We need to lock the slot against further inserts.
    while ( !__sync_bool_compare_and_swap( &(sl->rebuild), 0, 2 )) {
        sleep( 0 );
        sl = s->slots[idx];
    }

    nlist *nodes = nlist_create();
    if ( !nodes ) return 0;

    rstat check = nlist_push( nodes, sl->root );
    if ( check.bit.error ) goto RECONF_ERROR;

    node *n = nlist_shift( nodes );
    while( n != NULL ) {
        // If right is NULL we block it off with RBLD, otherwise add to list
        __sync_bool_compare_and_swap( &(n->right), NULL, RBLD );
        if ( n->right != RBLD ) {
            check = nlist_push( nodes, n->right );
            if ( check.bit.error ) goto RECONF_ERROR;
        }
        // If left is NULL we block it off with RBLD, otherwise add to list
        __sync_bool_compare_and_swap( &(n->left), NULL, RBLD );
        if ( n->left != RBLD ) {
            check = nlist_push( nodes, n->left );
            if ( check.bit.error ) goto RECONF_ERROR;
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

        goto RECONF_ERROR;
    }

    nlist_free( &nodes );
    return 1;

    RECONF_ERROR:
    nlist_free( &nodes );
    return 0;
}
