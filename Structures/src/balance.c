#include <stdio.h>
#include <string.h>

#include "include/gsd_dict_return_old.h"

#include "epoch.h"
#include "structure.h"
#include "balance.h"
#include "util.h"
#include "alloc.h"

const int XRBLD = 1;
const void *RBLD = &XRBLD;

int rebalance( dict *d, location *loc ) {
    // Attempt to create rebalance slot, or return
    slot *ns = malloc( sizeof( slot ));
    if ( ns == NULL ) return DICT_MEM_ERROR;
    memset( ns, 0, sizeof( slot ));

    // Create balance_pair array
    size_t size = loc->slot->count + 100;
    node **all = malloc( sizeof( node * ) * size );
    if ( all == NULL ) {
        free( ns );
        return DICT_MEM_ERROR;
    }
    memset( all, 0, size * sizeof( node * ));

    // Iterate nodes, add to array, block new branches
    size_t count = rebalance_node( loc->slot->root, &all, &size, 0 );
    if ( count == 0 ) return DICT_MEM_ERROR;
    ns->count = count;

    // insert nodes
    size_t ideal = max_bit( count );
    int ret = rebalance_insert_list( d, loc->set, ns, all, 0, count - 1, ideal );

    // swap
    if (!ret && __sync_bool_compare_and_swap( &(loc->set->slots[loc->slotn]), loc->slot, ns )) {
        dispose( d, loc->epoch, loc->set->settings->meta, loc->slot, SLOT );
    }
    else {
        free_slot( d, loc->set->settings->meta, ns );
    }

    free( all );
    return ret;
}

size_t rebalance_node( node *n, node ***all, size_t *size, size_t count ) {

    // If right is NULL we block it off with RBLD, otherwise recurse
    __sync_bool_compare_and_swap( &(n->right), NULL, RBLD );
    if ( n->right != RBLD ) count = rebalance_node( n->right, all, size, count );

    __sync_bool_compare_and_swap( &(n->usref->sref), NULL, RBLD );

    if ( n->usref->sref != RBLD ) {
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
    if ( n->left != RBLD ) count = rebalance_node( n->left, all, size, count );

    return count;
}

int rebalance_insert( dict *d, set *st, slot *s, node *n, size_t ideal ) {
    size_t height = 0;
    node **put_here = &(s->root);
    while ( *put_here != NULL ) {
        height++;
        int dir = d->methods->cmp( st->settings->meta, n->key, (*put_here)->key );
        if ( dir == -1 ) { // Left
            put_here = &((*put_here)->left);
        }
        else if ( dir == 1 ) { // Right
            put_here = &((*put_here)->right);
        }
        else {
            return DICT_API_ERROR;
        }
    }

    if ( height > ideal + st->settings->max_imbalance )
        return DICT_PATHO_ERROR;

    node *new_node = malloc( sizeof( node ));
    if ( new_node == NULL ) return DICT_MEM_ERROR;
    memset( new_node, 0, sizeof( node ));

    int success = 0;
    while ( !success ) {
        size_t c = n->usref->refcount;
        if ( c < 1 ) break;
        success = __sync_bool_compare_and_swap( &(n->usref->refcount), c, c + 1 );
    }
    if ( n->usref->sref == NULL || n->usref->sref->refcount < 1 ) {
        free( new_node );
        return DICT_NO_ERROR;
    }

    // REF TODO: key gains a ref
    new_node->key   = n->key;
    new_node->usref = n->usref;

    *put_here = new_node;
    return DICT_NO_ERROR;
}

int rebalance_insert_list( dict *d, set *st, slot *s, node **all, size_t start, size_t end, size_t ideal ) {
    if ( start == end ) return rebalance_insert( d, st, s, all[start], ideal );

    size_t total = end - start;
    size_t half = total / 2;
    size_t center = total % 2 ? start + half + 1
                              : start + half;

    int res = rebalance_insert( d, st, s, all[center], ideal );
    if ( res ) return res;

    res = rebalance_insert_list( d, st, s, all, start, center - 1, ideal );
    if ( res ) return res;

    if ( center == end ) return res;
    res = rebalance_insert_list( d, st, s, all, center + 1, end, ideal );
    return res;
}

