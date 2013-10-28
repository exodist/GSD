#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "devtools.h"
#include "structure.h"
#include "balance.h"
#include "util.h"
#include "alloc.h"

rstat rebalance( dict *d, set *st, size_t slotn, size_t *count_diff ) {
    slot *sl = NULL;
    __atomic_load( st->slots + slotn, &sl, __ATOMIC_CONSUME );
    if ( sl == NULL || blocked_null( sl )) return rstat_ok;
    int zero = 0;
    int succ = __atomic_compare_exchange_n(
        &(sl->rebuild),
        &zero,
        1,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_RELAXED
    );
    if (!succ) return rstat_ok;

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
    succ = __atomic_compare_exchange(
         st->slots + slotn,
         &old_slot,
         &ns,
         0,
         __ATOMIC_ACQ_REL,
         __ATOMIC_RELAXED
    );

    if ( succ ) {
        *count_diff = old_slot->item_count - count;
#ifdef METRICS
        __atomic_add_fetch( &(d->rebalanced), 1, __ATOMIC_ACQ_REL );
#endif
        dispose( d->prm, old_slot, free_slot, d );
        return rstat_ok;
    }

    REBALANCE_CLEANUP:
    rebalance_unblock( sl->root );
    if ( ns )  dispose( d->prm, ns, free_slot, d );
    if ( all ) free( all );
    int one = 1;
    assert( __atomic_compare_exchange_n(
        &(sl->rebuild),
        &one,
        0,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_RELAXED
    ));
    return out;
}

void rebalance_unblock( node *n ) {
    node *curr = (node *)&RBAL;

    int empty = __atomic_compare_exchange_n(
        &(n->right),
        &curr,
        NULL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );
    if ( !empty ) rebalance_unblock( curr );

    __atomic_compare_exchange_n(
        &(n->usref->sref),
        (void **)&RBAL,
        NULL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );

    curr = (node *)&RBAL;
    empty = __atomic_compare_exchange_n(
        &(n->left),
        &curr,
        NULL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );
    if ( !empty ) rebalance_unblock( curr );
}

size_t rebalance_add_node( node *n, node ***all, size_t *size, size_t count ) {
    // If right is NULL we block it off with RBAL, otherwise recurse
    node *curr = NULL;
    int empty = __atomic_compare_exchange_n(
        &(n->right),
        &curr,
        RBAL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );
    if ( !empty ) count = rebalance_add_node( curr, all, size, count );

    // Mark a node with no sref as rebalance
    sref *csref = NULL;
    empty = __atomic_compare_exchange_n(
        &(n->usref->sref),
        &csref,
        RBAL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );

    // If this node has an sref, add to the list.
    if ( !empty ) {
        if ( count >= *size ) {
            node **nall = realloc( *all, (*size + 10) * sizeof(node *));
            *size += 10;
            if ( nall == NULL ) return 0;
            *all = nall;
        }
        (*all)[count] = n;
        count++;
    }

    // If left is NULL we block it off with RBAL, otherwise recurse
    curr = NULL;
    empty = __atomic_compare_exchange_n(
        &(n->left),
        &curr,
        RBAL,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );
    if ( !empty ) count = rebalance_add_node( curr, all, size, count );

    return count;
}

rstat rebalance_insert( dict *d, set *st, slot **s, node *n, size_t ideal ) {
    // Copy the key, xtrn structs are not refcounted, they go away with their
    // container, so for node cloning the xtrn must also be cloned.
    void *key = create_xtrn( d, n->key );
    if ( !key ) return rstat_mem;

    node *new_node = create_node( key, n->usref, 1 );
    if ( !new_node ) {
        dispose( d->prm, key, free_xtrn, d );
        return rstat_mem;
    }

    // If this is the root node, create the slot with the new node as root.
    if ( *s == NULL ) {
        *s = create_slot( new_node );
        if ( *s == NULL ) {
            dispose( d->prm, new_node, free_node, d );
            return rstat_mem;
        }
        return rstat_ok;
    }

    location *loc = NULL;
    rstat stat = locate_from_node( d, key, &loc, st, (*s)->root );
    if ( stat.bit.error ) {
        if ( loc ) free_location( d, loc );
        dispose( d->prm, new_node, free_node, d );
        return stat;
    }
    dev_assert( loc );
    dev_assert( loc->parent );
    dev_assert( loc->dir );

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
    // If max_imbalance is 0 we don't check
    if ( !loc->set->settings.max_imbalance ) return rstat_ok;

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
        size_t count_diff = 0;
        rstat ret = rebalance( d, loc->set, loc->slotn, &count_diff );
        __sync_bool_compare_and_swap( &(loc->slot->rebuild), 1, 0 );

        if ( ret.bit.error ) {
            ret.bit.fail  = 0;
            ret.bit.rebal = 1;
            return ret;
        }
        else { // update all count
            __sync_fetch_and_sub( &(d->item_count), count_diff );
        }

        // This only happens if the application can have multiple items that
        // are identical but do not compare as equal, forcing it to always
        // return a default direction. Ideally the application would avoid
        // this. In such cases it is unlikely any algorithm, seed, salt, etc
        // changes will be helpful.
        if ( loc->slot->patho ) return rstat_patho;
    }

    // Check if we are balanced with our neighbors
    size_t us = loc->slotn;
    size_t left  = (us - 1) % loc->set->settings.slot_count;
    size_t right = (us + 1) % loc->set->settings.slot_count;
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

rstat rebalance_all( dict *d, size_t threads ) {
    uint8_t e = join_epoch( d->prm );
    set *s = d->set;

    void *args[1] = { d };

    rstat out = threaded_traverse(
        s, 0, s->settings.slot_count,
        rebalance_callback,
        args,
        threads
    );

    leave_epoch( d->prm, e );
    return out;
}

rstat rebalance_callback( set *s, size_t idx, void **cb_args ) {
    dict *d = cb_args[0];
    size_t count_diff = 0;

    rstat out = rebalance( d, s, idx, &count_diff );
    __sync_fetch_and_sub( &(d->item_count), count_diff );

    return out;
}
