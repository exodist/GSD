#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_free.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_balance.h"

void dict_free_set( dict *d, set *s ) {
    for ( int i = 0; i < s->slot_count; i++ ) {
        if ( s->slots[i] != NULL ) dict_free_slot( d, s->meta, s->slots[i] );
    }
    free( s );
}

void dict_free_slot( dict *d, void *meta, slot *s ) {
    if ( s->root != NULL ) dict_free_node( d, meta, s->root );
    free( s );
}

void dict_free_node( dict *d, void *meta, node *n ) {
    if ( n->left != NULL && n->left != RBLD )
        dict_free_node( d, meta, n->left );
    if ( n->right != NULL && n->right != RBLD )
        dict_free_node( d, meta, n->right );

    size_t count = __sync_sub_and_fetch( &(n->value->refcount), 1 );
    if( count == 0 ) {
        if ( n->value->value != NULL && n->value->value != RBLD ) {
            sref *r = n->value->value;
            count = __sync_sub_and_fetch( &(r->refcount), 1 );
            if( count == 0 ) dict_free_sref( d, r );
            if ( d->methods->rem != NULL )
                d->methods->rem( d, meta, n->key, n->value->value->value );
        }
        else {
            if ( d->methods->rem != NULL )
                d->methods->rem( d, meta, n->key, NULL );
        }

        free( n->value );
    }
}

void dict_free_sref( dict *d, sref *r ) {
    free( r );
}


