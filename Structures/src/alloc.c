#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "devtools.h"
#include "structure.h"
#include "alloc.h"
#include "epoch.h"
#include "balance.h"
#include "error.h"

rstat do_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    // Wait on all epochs
    for ( uint8_t i = 0; i < EPOCH_LIMIT; i++ ) {
        while( d->epochs[i].active ) sleep( 0 );
    }

    // Wait for garbage collection threads
    while ( d->detached_threads ) sleep( 0 );

    if ( d->set != NULL ) free_set( d, d->set );

    free( d );
    return rstat_ok;
}

void free_trash( dict *d, trash *t ) {
    while ( t != NULL ) {
        trash *goner = t;
        t = t->next;

        dev_assert( goner->type );
        switch ( goner->type ) {
            // Not reachable, OOPS = 0, will be caught by the dev_assert above. This is
            // here to silence a warning.
            case OOPS:
                return;

            case SET:
                free_set( d, (set *)goner );
            break;
            case SLOT:
                free_slot( d, (slot *)goner );
            break;
            case NODE:
                free_node( d, (node *)goner );
            break;
            case SREF:
                free_sref( d, (sref *)goner );
            break;
            case XTRN:
                free_xtrn( d, (xtrn *)goner );
            break;
        }
    }
}

void free_set( dict *d, set *s ) {
    for ( int i = 0; i < s->settings.slot_count; i++ ) {
        if ( s->slots[i] != NULL ) free_slot( d, s->slots[i] );
    }
    free( s->slots );
    free( s );
}

void free_slot( dict *d, slot *s ) {
    if ( s->root != NULL ) free_node( d, s->root );
    free( s );
}

void free_node( dict *d, node *n ) {
    if ( n->left && !blocked_null( n->left ))
        free_node( d, n->left );
    if ( n->right && !blocked_null( n->right ))
        free_node( d, n->right );

    size_t count = __sync_sub_and_fetch( &(n->usref->refcount), 1 );
    if( count == 0 ) {
        sref *r = n->usref->sref;
        if ( r && !blocked_null( r )) {
            count = __sync_sub_and_fetch( &(r->refcount), 1 );
            // If refcount is SIZE_MAX we almost certainly have an underflow. 
            dev_assert( count != SIZE_MAX );
            if( count == 0 ) free_sref( d, r );
        }

        free( n->usref );
    }

    free_xtrn( d, n->key );

    free( n );
}

void free_sref( dict *d, sref *r ) {
    if ( r->xtrn && !blocked_null( r->xtrn ))
        free_xtrn( d, r->xtrn );

    free( r );
}

void free_xtrn( dict *d, xtrn *x ) {
    if ( d->methods.ref && x->value )
        d->methods.ref( d, x->value, -1 );

    free( x );
}

rstat do_create( dict **d, dict_settings settings, dict_methods methods ) {
    if ( methods.cmp == NULL ) return error( 1, 0, DICT_API_MISUSE, "The 'cmp' method may not be NULL.", 0 );
    if ( methods.loc == NULL ) return error( 1, 0, DICT_API_MISUSE, "The 'loc' method may not be NULL.", 0 );

    if( !settings.slot_count )    settings.slot_count    = 128;

    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return rstat_mem;
    memset( out, 0, sizeof( dict ));

    out->set = create_set( settings, settings.slot_count );
    if ( out->set == NULL ) {
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

    out->trash.type = SET;

    return out;
}

slot *create_slot( node *root ) {
    dev_assert( root );
    slot *new_slot = malloc( sizeof( slot ));
    if ( !new_slot ) return NULL;
    memset( new_slot, 0, sizeof( slot ));
    new_slot->root = root;
    new_slot->trash.type = SLOT;

    return new_slot;
}

node *create_node( xtrn *key, usref *ref, size_t min_start_refcount ) {
    dev_assert( ref );
    dev_assert( key );

    node *new_node = malloc( sizeof( node ));
    if ( !new_node ) return NULL;
    memset( new_node, 0, sizeof( node ));
    new_node->key = key;
    size_t rc;
    while ( 1 ) {
        rc = ref->refcount;
        if ( rc < min_start_refcount ) {
            free( new_node );
            return NULL;
        }

        if ( __sync_bool_compare_and_swap( &(ref->refcount), rc, rc + 1 ))
            break;
    }
    dev_assert( ref->refcount );
    new_node->usref = ref;
    new_node->trash.type = NODE;

    return new_node;
}

usref *create_usref( sref *ref ) {
    usref *new_usref = malloc( sizeof( usref ));
    if ( !new_usref ) return NULL;
    memset( new_usref, 0, sizeof( usref ));
    if ( ref ) {
        __sync_add_and_fetch( &(ref->refcount), 1 );
    }
    new_usref->sref = ref;
    return new_usref;
}

sref *create_sref( xtrn *x, dict_trigger *t ) {
    sref *new_sref = malloc( sizeof( sref ));
    if ( !new_sref ) return NULL;
    memset( new_sref, 0, sizeof( sref ));
    new_sref->xtrn = x;
    new_sref->trash.type = SREF;
    new_sref->trigger = t;

    return new_sref;
}

xtrn *create_xtrn( dict *d, void *value ) {
    dev_assert( value );
    dev_assert( d );
    xtrn *new_xtrn = malloc( sizeof( xtrn ));
    if ( !new_xtrn ) return NULL;
    memset( new_xtrn, 0, sizeof( xtrn ));

    if ( d->methods.ref )
        d->methods.ref( d, value, 1 );

    new_xtrn->value = value;
    new_xtrn->trash.type = XTRN;

    return new_xtrn;
}

