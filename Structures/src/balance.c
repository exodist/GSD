#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "epoch.h"
#include "structure.h"
#include "balance.h"
#include "util.h"
#include "alloc.h"

const int XRBLD = 1;
const void *RBLD = &XRBLD;

rstat rebalance( dict *d, set *st, size_t slotn ) {
    slot *sl = st->slots[slotn];
    if( !__sync_bool_compare_and_swap( &(sl->rebuild), 0, 1 ))
        return rstat_ok;

    rstat out = rstat_ok;

    // Create balance_pair array
    size_t size = sl->item_count + 100;
    node **all = malloc( sizeof( node * ) * size );
    if ( all == NULL ) {
        out = rstat_mem;
        goto REBALANCE_CLEANUP;
    };
    memset( all, 0, size * sizeof( node * ));

    // Iterate nodes, add to array, block new branches
    size_t count = rebalance_add_node( sl->root, &all, &size, 0 );
    if ( count == 0 ) {
        out = rstat_mem;
        goto REBALANCE_CLEANUP;
    }

    slot *ns = NULL;

    // insert nodes
    size_t ideal = max_bit( count );
    out = rebalance_insert_list( d, st, &ns, all, 0, count - 1, ideal );
    free( all );
    all = NULL;

    if ( out.bit.error )
        goto REBALANCE_CLEANUP;

    ns->item_count = count;

    // swap
    slot *old_slot = sl;
    int success = __sync_bool_compare_and_swap( &(st->slots[slotn]), old_slot, ns );

    if ( success ) {
        __sync_add_and_fetch( &(d->rebalanced), 1 );
        dispose( d, (trash *)old_slot );
        return rstat_ok;
    }
    else {
        sl = old_slot;
    }

    REBALANCE_CLEANUP:
    rebalance_unblock( sl->root );
    if ( ns )  dispose( d, (trash *)ns );
    if ( all ) free( all );
    __sync_bool_compare_and_swap( &(sl->rebuild), 1, 0 );
    return out;
}

void rebalance_unblock( node *n ) {
    __sync_bool_compare_and_swap( &(n->right), RBLD, NULL );
    if ( n->right != NULL ) rebalance_unblock( n->right );

    __sync_bool_compare_and_swap( &(n->usref->sref), RBLD, NULL );

    __sync_bool_compare_and_swap( &(n->left), RBLD, NULL );
    if ( n->left != NULL ) rebalance_unblock( n->left );
}

size_t rebalance_add_node( node *n, node ***all, size_t *size, size_t count ) {
    // If right is NULL we block it off with RBLD, otherwise recurse
    __sync_bool_compare_and_swap( &(n->right), NULL, RBLD );
    if ( n->right != RBLD ) count = rebalance_add_node( n->right, all, size, count );

    // Mark a node with no sref as rebuild
    __sync_bool_compare_and_swap( &(n->usref->sref), NULL, RBLD );

    // If this node has an sref, add to the list.
    sref *sr = n->usref->sref;
    if ( sr != RBLD ) {
        if ( count >= *size ) {
            node **nall = realloc( *all, (*size + 10) * sizeof(node *));
            *size += 10;
            if ( nall == NULL ) return 0;
            *all = nall;
        }
        (*all)[count] = n;
        count++;
    }

    // If left is NULL we block it off with RBLD, otherwise recurse
    __sync_bool_compare_and_swap( &(n->left), NULL, RBLD );
    if ( n->left != RBLD ) count = rebalance_add_node( n->left, all, size, count );

    return count;
}

rstat rebalance_insert( dict *d, set *st, slot **s, node *n, size_t ideal ) {
    // Copy the key, xtrn structs are not refcounted, they go away with their
    // container, so for node cloning the xtrn must also be cloned.
    xtrn *key = create_xtrn( d, n->key->value );
    if ( !key ) return rstat_mem;

    node *new_node = create_node( key, n->usref, 1 );
    if ( !new_node ) {
        dispose( d, (trash *)key );
        return rstat_mem;
    }

    // If this is the root node, create the slot with the new node as root.
    if ( *s == NULL ) {
        *s = create_slot( new_node );
        if ( *s == NULL ) {
            dispose( d, (trash *)new_node );
            return rstat_mem;
        }
        return rstat_ok;
    }

    location *loc = NULL;
    rstat stat = locate_from_node( d, key->value, &loc, st, (*s)->root );
    if ( stat.bit.error ) {
        if ( loc ) free_location( d, loc );
        dispose( d, (trash *)new_node );
        return stat;
    }
    assert( loc );
    assert( loc->parent );
    assert( loc->dir );

    if ( loc->dir == -1 ) {
        loc->parent->left = new_node;
    }
    else if ( loc->dir == 1 ) {
        loc->parent->right = new_node;
    }

    if ( loc->height > ( ideal + st->settings.max_imbalance ))
        (*s)->patho = 1;

    free_location( d, loc );
    return rstat_ok;
}

rstat rebalance_insert_list( dict *d, set *st, slot **s, node **all, size_t start, size_t end, size_t ideal ) {
    if ( start == end ) return rebalance_insert( d, st, s, all[start], ideal );

    size_t total = end - start;
    size_t half = total / 2;
    size_t center = total % 2 ? start + half + 1
                              : start + half;

    rstat res = rebalance_insert( d, st, s, all[center], ideal );
    if ( res.num ) return res;

    res = rebalance_insert_list( d, st, s, all, start, center - 1, ideal );
    if ( res.num ) return res;

    if ( center == end ) return res;
    res = rebalance_insert_list( d, st, s, all, center + 1, end, ideal );
    return res;
}

rstat balance_check( dict *d, location *loc, size_t count ) {
    // If the data is known-pathological, don't bother trying to balance it.
    if ( loc->slot->patho ) return rstat_patho;

    // Find ideal height
    size_t ideal = (size_t)max_bit( count );

    // Update the ideal height in the slot.
    uint8_t old_ideal = loc->slot->ideal_height;
    while ( ideal > old_ideal && count >= loc->slot->item_count ) {
        if( !__sync_bool_compare_and_swap( &(loc->slot->ideal_height), old_ideal, ideal ))
            old_ideal = loc->slot->ideal_height;
    }

    size_t height  = loc->height;
    size_t max_imb = loc->set->settings.max_imbalance;

    // Check if we have an internal imbalance
    if ( height > (ideal + max_imb) && !loc->slot->rebuild ) {
        rstat ret = rebalance( d, loc->set, loc->slotn );
        __sync_bool_compare_and_swap( &(loc->slot->rebuild), 1, 0 );

        if ( ret.bit.error ) {
            ret.bit.fail  = 0;
            ret.bit.rebal = 1;
            return ret;
        }

        if ( loc->slot->patho ) return rstat_patho;
    }

    // Check if we are balanced with our neighbors
    size_t us = loc->slotn;
    size_t left  = (us - 1) % loc->set->slot_count;
    size_t right = (us + 1) % loc->set->slot_count;
    size_t neighbors[2] = { left, right };

    for ( size_t ni = 0; ni < 2; ni++ ) {
        if ( ni == us ) continue;

        slot *ns = loc->set->slots[neighbors[ni]];

        size_t n_ideal = ns ? ns->ideal_height : 0;
        if ( ideal > (n_ideal + ( max_imb * 10 ))) {
            // Do not set slot->patho, this form of pathological simply means
            // excess nodes are joining this slot, balancing the slot may still
            // be helpful.
            // loc->slot->patho = 1;
            return rstat_patho;
        }
    }

    return rstat_ok;
}

dict_stat rebalance_all( dict *d, size_t threshold, size_t threads ) {
    epoch *e = join_epoch( d );
    set *s = d->set;

    size_t index = 0;
    void *args[3] = { &index, s, d };

    if ( threads == 1 ) {
        rebalance_worker( args );
    }
    else {
        pthread_t *pts = malloc( threads * sizeof( pthread_t ));
        for ( int i = 0; i < threads; i++ ) {
            pthread_create( &(pts[i]), NULL, rebalance_worker, args );
        }
        for ( int i = 0; i < threads; i++ ) {
            pthread_join( pts[i], NULL );
        }
    }

    leave_epoch( d, e );
    return rstat_ok;
}

void *rebalance_worker( void *in ) {
    void **args = (void **)in;
    size_t *index = args[0];
    set    *s     = args[1];
    dict   *d     = args[2];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= s->slot_count ) return NULL;
        rebalance( d, s, idx );
    }
}

