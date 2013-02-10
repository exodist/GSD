#include <unistd.h>
#include <string.h>

#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"

#include "structure.h"
#include "alloc.h"
#include "epoch.h"
#include "balance.h"

int do_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    // Wait on all epochs
    epoch *e = d->epochs;
    while ( e != NULL ) {
        while( e->active ) sleep( 0 );
        e = e->next;
    }

    if ( d->set != NULL ) free_set( d, d->set );
    free( d );

    return DICT_NO_ERROR;
}

void free_set( dict *d, set *s ) {
    for ( int i = 0; i < s->settings->slot_count; i++ ) {
        if ( s->slots[i] != NULL ) free_slot( d, s->settings->meta, s->slots[i] );
    }
    free( s );
}

void free_slot( dict *d, void *meta, slot *s ) {
    if ( s->root != NULL ) free_node( d, meta, s->root );
    free( s );
}

void free_node( dict *d, void *meta, node *n ) {
    if ( n->left != NULL && n->left != RBLD )
        free_node( d, meta, n->left );
    if ( n->right != NULL && n->right != RBLD )
        free_node( d, meta, n->right );

    size_t count = __sync_sub_and_fetch( &(n->usref->refcount), 1 );
    if( count == 0 ) {
        sref *r = n->usref->sref;
        if ( r != NULL && r != RBLD ) {
            count = __sync_sub_and_fetch( &(r->refcount), 1 );
            if( count == 0 ) free_sref( d, meta, r );
        }

        free( n->usref );
    }

    // REF TODO: key loses a ref

    free( n );
}

void free_sref( dict *d, void *meta, sref *r ) {
    // REF TODO: value loses a ref
    //if ( r->value != NULL && r->value != RBLD && d->methods->ref_del != NULL )
    //    d->methods->ref_del( d, meta, r->value );

    free( r );
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

    return out;
}

int do_create( dict **d, uint8_t epoch_limit, dict_settings *settings, dict_methods *methods ) {
    if ( settings == NULL )     return DICT_API_ERROR;
    if ( methods == NULL )      return DICT_API_ERROR;
    if ( methods->cmp == NULL ) return DICT_API_ERROR;
    if ( methods->loc == NULL ) return DICT_API_ERROR;
    if ( epoch_limit && epoch_limit < 4 ) return DICT_API_ERROR;

    if( !settings->slot_count    ) settings->slot_count    = 256;
    if( !settings->max_imbalance ) settings->max_imbalance = 3;

    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return DICT_MEM_ERROR;
    memset( out, 0, sizeof( dict ));

    out->epoch_limit = epoch_limit;
    out->epochs = create_epoch();
    out->epoch = out->epochs;
    out->epoch_count = 2;
    if ( out->epochs == NULL ) {
        free( out );
        return DICT_MEM_ERROR;
    }

    out->epochs->next = create_epoch();
    if ( out->epochs->next == NULL ) {
        free( out->epochs );
        free( out );
        return DICT_MEM_ERROR;
    }

    out->set = create_set( settings );
    if ( out->set == NULL ) {
        free( out->epochs );
        free( out );
        return DICT_MEM_ERROR;
    }

    out->methods = methods;

    *d = out;

    return DICT_NO_ERROR;
}
