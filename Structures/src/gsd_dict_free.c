#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_free.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_balance.h"

void dict_free_set( dict *d, set *s ) {
    for ( int i = 0; i < s->settings->slot_count; i++ ) {
        if ( s->slots[i] != NULL ) dict_free_slot( d, s->settings->meta, s->slots[i] );
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

    size_t count = __sync_sub_and_fetch( &(n->usref->refcount), 1 );
    if( count == 0 ) {
        sref *r = n->usref->sref;
        if ( r != NULL && r != RBLD ) {
            count = __sync_sub_and_fetch( &(r->refcount), 1 );
            if( count == 0 ) dict_free_sref( d, meta, r );
        }

        if ( d->methods->rem != NULL )
            d->methods->rem( d, meta, n->key );

        free( n->usref );
    }

    if ( d->methods->ref_del != NULL )
        d->methods->ref_del( d, meta, n->key );

    free( n );
}

void dict_free_sref( dict *d, void *meta, sref *r ) {
    if ( r->value != NULL && r->value != RBLD && d->methods->ref_del != NULL )
        d->methods->ref_del( d, meta, r->value );

    free( r );
}


