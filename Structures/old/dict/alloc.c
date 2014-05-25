#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "../../PRM/src/include/gsd_prm.h"

#include "devtools.h"
#include "structure.h"
#include "alloc.h"
#include "balance.h"
#include "error.h"

rstat do_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    while(!free_prm( d->prm )) {
        sleep(0);
    }

    if ( d->set != NULL ) free_set( d->set, d );

    free( d );
    return rstat_ok;
}

void free_set( void *ptr, void *arg ) {
    set  *s = ptr;
    dict *d = arg;
    for ( int i = 0; i < s->settings.slot_count; i++ ) {
        if ( s->slots[i] != NULL ) free_slot( s->slots[i], d );
    }
    free( s->slots );
    free( s );
}

void free_slot( void *ptr, void *arg ) {
    slot *s = ptr;
    dict *d = arg;
    if ( s->root != NULL ) free_node( s->root, d );
    free( s );
}

void free_node( void *ptr, void *arg ) {
    node *n = ptr;
    dict *d = arg;

    if ( n->left && !blocked_null( n->left ))
        free_node( n->left, d );
    if ( n->right && !blocked_null( n->right ))
        free_node( n->right, d );

    size_t count = __atomic_sub_fetch( &(n->usref->refcount), 1, __ATOMIC_ACQ_REL );
    if( count == 0 ) {
        sref *r = n->usref->sref;
        if ( r && !blocked_null( r )) {
            count = __atomic_sub_fetch( &(r->refcount), 1, __ATOMIC_ACQ_REL );
            // If refcount is SIZE_MAX we almost certainly have an underflow.
            dev_assert( count != SIZE_MAX );
            if( count == 0 ) free_sref( r, d );
        }

        free( n->usref );
    }

    free_xtrn( n->key, d );

    free( n );
}

rstat do_create( dict **d, dict_settings settings, dict_methods methods ) {
    if ( methods.cmp == NULL ) return error( 1, 0, DICT_API_MISUSE, "The 'cmp' method may not be NULL." );
    if ( methods.loc == NULL ) return error( 1, 0, DICT_API_MISUSE, "The 'loc' method may not be NULL." );

    if( !settings.slot_count )    settings.slot_count    = 128;

    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return rstat_mem;
    memset( out, 0, sizeof( dict ));

    out->prm = build_prm( 5, 63, 100 );
    if (!out->prm) {
        free( out );
        return rstat_mem;
    }

    out->set = create_set( settings, settings.slot_count );
    if ( out->set == NULL ) {
        free_prm( out->prm );
        free( out );
        return rstat_mem;
    }

    out->methods = methods;

    *d = out;

    return rstat_ok;
}

set *create_set( dict_settings settings, size_t slot_count ) {
    set *out = malloc( sizeof( set ));
    if ( out == NULL ) return NULL;

    memset( out, 0, sizeof( set ));

    out->settings = settings;
    out->slots = malloc( slot_count * sizeof( slot * ));
    if ( out->slots == NULL ) {
        free( out );
        return NULL;
    }

    memset( out->slots, 0, slot_count * sizeof( slot * ));
    out->settings.slot_count = slot_count;

    return out;
}

slot *create_slot( node *root ) {
    dev_assert( root );
    slot *new_slot = malloc( sizeof( slot ));
    if ( !new_slot ) return NULL;
    memset( new_slot, 0, sizeof( slot ));
    new_slot->root = root;

    return new_slot;
}

node *create_node( void *key, usref *ref, size_t min_start_refcount ) {
    dev_assert( ref );
    dev_assert( key );

    node *new_node = malloc( sizeof( node ));
    if ( !new_node ) return NULL;
    memset( new_node, 0, sizeof( node ));
    new_node->key = key;
    size_t rc = __atomic_load_n( &(ref->refcount), __ATOMIC_CONSUME );
    while ( 1 ) {
        if ( rc < min_start_refcount ) {
            free( new_node );
            return NULL;
        }

        int res = __atomic_compare_exchange_n(
            &(ref->refcount),
            &rc,
            rc + 1,
            0,
            __ATOMIC_RELEASE,
            __ATOMIC_CONSUME
        );
        if (res) break;
    }
    dev_assert( __atomic_load_n( &(ref->refcount), __ATOMIC_CONSUME ));
    new_node->usref = ref;

    return new_node;
}

usref *create_usref( sref *ref ) {
    usref *new_usref = malloc( sizeof( usref ));
    if ( !new_usref ) return NULL;
    memset( new_usref, 0, sizeof( usref ));
    if ( ref ) {
        __atomic_add_fetch( &(ref->refcount), 1, __ATOMIC_ACQ_REL );
    }
    new_usref->sref = ref;
    return new_usref;
}

