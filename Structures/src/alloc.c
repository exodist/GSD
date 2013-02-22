#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "structure.h"
#include "alloc.h"
#include "epoch.h"
#include "balance.h"
#include "error.h"

rstat do_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    // Wait on all epochs
    epoch *e = d->epochs;
    while ( e != NULL ) {
        while( e->active ) sleep( 0 );
        e = e->next;
    }

    if ( d->set != NULL ) free_set( d, d->set );

    e = d->epochs;
    d->epochs = NULL;
    while ( e != NULL ) {
        epoch *goner = e;
        e = e->next;
        free( goner );
    }

    free( d );
    return rstat_ok;
}

void free_trash( dict *d, trash *t ) {
    while ( t != NULL ) {
        trash *goner = t;
        t = t->next;

        assert( goner->type );
        switch ( goner->type ) {
            // Not reachable, OOPS = 0, will be caught by the assert above. This is
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
    for ( int i = 0; i < s->settings->slot_count; i++ ) {
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
    if ( n->left != NULL && n->left != RBLD )
        free_node( d, n->left );
    if ( n->right != NULL && n->right != RBLD )
        free_node( d, n->right );

    size_t count = __sync_sub_and_fetch( &(n->usref->refcount), 1 );
    if( count == 0 ) {
        sref *r = n->usref->sref;
        if ( r != NULL && r != RBLD ) {
            count = __sync_sub_and_fetch( &(r->refcount), 1 );
            // If refcount is SIZE_MAX we almost certainly have an underflow. 
            assert( count != SIZE_MAX );
            if( count == 0 ) free_sref( d, r );
        }

        free( n->usref );
    }

    free_xtrn( d, n->key );

    free( n );
}

void free_sref( dict *d, sref *r ) {
    if ( r->xtrn && r->xtrn != RBLD )
        free_xtrn( d, r->xtrn );

    free( r );
}

void free_xtrn( dict *d, xtrn *x ) {
    if ( d->methods->ref && x->value )
        d->methods->ref( d, x->value, -1 );

    free( x );
}

rstat do_create( dict **d, uint8_t epoch_limit, dict_settings *settings, dict_methods *methods ) {
    if ( settings == NULL )     return error( 1, 0, DICT_API_MISUSE, 5 );
    if ( methods == NULL )      return error( 1, 0, DICT_API_MISUSE, 6 );
    if ( methods->cmp == NULL ) return error( 1, 0, DICT_API_MISUSE, 7 );
    if ( methods->loc == NULL ) return error( 1, 0, DICT_API_MISUSE, 8 );

    if ( epoch_limit && epoch_limit < 4 )
        return error( 1, 0, DICT_API_MISUSE, 9 );

    if( !settings->slot_count    ) settings->slot_count    = 256;
    if( !settings->max_imbalance ) settings->max_imbalance = 3;

    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return rstat_mem;
    memset( out, 0, sizeof( dict ));

    out->epoch_limit = epoch_limit;
    out->epochs = create_epoch();
    out->epoch = out->epochs;
    out->epoch_count = 2;
    if ( out->epochs == NULL ) {
        free( out );
        return rstat_mem;
    }

    out->epochs->next = create_epoch();
    if ( out->epochs->next == NULL ) {
        free( out->epochs );
        free( out );
        return rstat_mem;
    }

    out->set = create_set( settings );
    if ( out->set == NULL ) {
        free( out->epochs );
        free( out );
        return rstat_mem;
    }

    out->methods = methods;

    *d = out;

    return rstat_ok;
}

set *create_set( dict_settings *settings ) {
    set *out = malloc( sizeof( set ));
    if ( out == NULL ) return NULL;

    memset( out, 0, sizeof( set ));

    out->settings = settings;
    out->slots = malloc( settings->slot_count * sizeof( slot * ));
    if ( out->slots == NULL ) {
        free( out );
        return NULL;
    }

    memset( out->slots, 0, settings->slot_count * sizeof( slot * ));

    out->trash.type = SET;

    return out;
}

slot *create_slot( node *root ) {
    assert( root );
    slot *new_slot = malloc( sizeof( slot ));
    if ( !new_slot ) return NULL;
    memset( new_slot, 0, sizeof( slot ));
    new_slot->root = root;
    new_slot->trash.type = SLOT;

    return new_slot;
}

node *create_node( xtrn *key, usref *ref, size_t min_start_refcount ) {
    assert( ref );
    assert( key );

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
    assert( ref->refcount );
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

sref *create_sref( xtrn *x ) {
    sref *new_sref = malloc( sizeof( sref ));
    if ( !new_sref ) return NULL;
    memset( new_sref, 0, sizeof( sref ));
    new_sref->xtrn = x;
    new_sref->trash.type = SREF;

    return new_sref;
}

xtrn *create_xtrn( dict *d, void *value ) {
    assert( value );
    assert( d );
    xtrn *new_xtrn = malloc( sizeof( xtrn ));
    if ( !new_xtrn ) return NULL;
    memset( new_xtrn, 0, sizeof( xtrn ));

    if ( d->methods->ref )
        d->methods->ref( d, value, 1 );

    new_xtrn->value = value;
    new_xtrn->trash.type = XTRN;

    return new_xtrn;
}

