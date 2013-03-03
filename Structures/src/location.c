#include <string.h>
#include <assert.h>

#include "epoch.h"
#include "structure.h"
#include "location.h"
#include "balance.h"

location *create_location( dict *d ) {
    location *locate = malloc( sizeof( location ));
    if ( locate == NULL ) return NULL;
    memset( locate, 0, sizeof( location ));

    locate->epoch = join_epoch( d );

    return locate;
}

void free_location( dict *d, location *locate ) {
    leave_epoch( d, locate->epoch );
    free( locate );
}

rstat locate_key( dict *d, void *key, location **locate ) {
    if ( *locate == NULL ) {
        *locate = create_location( d );
        if ( *locate == NULL ) return rstat_mem;
    }
    location *lc = *locate;

    // We always reset these if we are reusing the location data.
    lc->dir   = 0;
    lc->node  = NULL;
    lc->usref = NULL;
    lc->sref  = NULL;
    lc->xtrn  = NULL;

    // The set has been swapped start over.
    if ( lc->set != NULL && lc->set != d->set ) {
        lc->set       = NULL;
        lc->slotn_set = 0;
        lc->slotn     = 0;
        lc->slot      = NULL;
        lc->parent    = NULL;
    }

    if ( lc->set == NULL ) {
        lc->set = d->set;
    }

    if ( !lc->slotn_set ) {
        lc->slotn = d->methods.loc( lc->set->settings.slot_count, lc->set->settings.meta, key );
        lc->slotn_set = 1;
    }

    // If the slot has been swapped use the new one (resets decendant values)
    if ( lc->slot != NULL && lc->slot != lc->set->slots[lc->slotn] ) {
        lc->slot    = NULL;
        lc->parent  = NULL;
    }

    if ( lc->slot == NULL ) {
        slot *slt = lc->set->slots[lc->slotn];

        // Slot is not populated
        if ( slt == NULL || blocked_null( slt )){
            lc->parent = NULL;
            return rstat_ok;
        }

        lc->slot = slt;
    }

    if ( lc->parent == NULL ) {
        lc->parent = lc->slot->root;
        if ( lc->parent == NULL || blocked_null( lc->parent )) {
            lc->height = 0;
            lc->parent = NULL;
            return rstat_ok;
        }
        else {
            lc->height = 1;
        }
    }

    return locate_from_node( d, key, locate, lc->set, lc->parent );
}

rstat locate_from_node( dict *d, void *key, location **locate, set *s, node *in ) {
    if ( *locate == NULL ) {
        *locate = create_location( d );
        if ( *locate == NULL ) return rstat_mem;
    }
    location *lc = *locate;

    // Assure we have a set.
    if ( !lc->set ) lc->set = s;
    assert( lc->set );
    assert( in );
    assert( key );

    lc->node  = NULL;
    lc->usref = NULL;
    lc->sref  = NULL;
    lc->xtrn  = NULL;

    node *n = in;
    while ( n != NULL && !blocked_null( n )) {
        int dir = d->methods.cmp( lc->set->settings.meta, key, n->key->value );
        switch( dir ) {
            case 0:
                lc->node = n;

                if ( lc->set->immutable ) {
                    lc->xtrn = n->value.xtrn;
                    return rstat_ok;
                }

                lc->usref = lc->node->value.usref; // This is never NULL
                lc->sref  = lc->usref->sref;
                lc->xtrn  = lc->sref && !blocked_null( lc->sref ) ? lc->sref->xtrn : NULL;

                // If the node has a rebuild value we do not want to use it.
                if ( blocked_null( lc->sref )) {
                    lc->sref = NULL;
                    lc->xtrn = NULL;
                }

                return rstat_ok;
            break;
            case -1:
                lc->dir = dir;
                lc->parent = n;
                lc->height++;
                n = n->left;
            break;
            case 1:
                lc->dir = dir;
                lc->parent = n;
                lc->height++;
                n = n->right;
            break;
            default:
                return error( 1, 0, DICT_API_MISUSE, 10, 0 );
            break;
        }
    }
    assert( lc->parent || lc->node == in );

    return rstat_ok;
}


