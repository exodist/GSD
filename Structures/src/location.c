#include <string.h>

#include "include/gsd_dict.h"
#include "include/gsd_dict_return_old.h"

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

int locate_key( dict *d, void *key, location **locate ) {
    if ( *locate == NULL ) {
        *locate = create_location( d );
        if ( *locate == NULL ) return DICT_MEM_ERROR;
    }
    location *lc = *locate;

    // The set has been swapped start over.
    if ( lc->set != NULL && lc->set != d->set ) {
        memset( lc, 0, sizeof( location ));
    }

    if ( lc->set == NULL ) {
        lc->set = d->set;
    }

    if ( !lc->slotn_set ) {
        lc->slotn  = d->methods->loc( lc->set->settings, key );
        lc->slotn_set = 1;
    }

    // If the slot has been swapped use the new one (resets decendant values)
    if ( lc->slot != NULL && lc->slot != lc->set->slots[lc->slotn] ) {
        lc->slot    = NULL;
        lc->parent = NULL;
        lc->node  = NULL;
        lc->usref  = NULL;
        lc->sref   = NULL;
    }

    if ( lc->slot == NULL ) {
        slot *slt = lc->set->slots[lc->slotn];

        // Slot is not populated
        if ( slt == NULL || slt == RBLD ){
            lc->parent = NULL;
            lc->node  = NULL;
            lc->usref  = NULL;
            lc->sref   = NULL;
            return DICT_NO_ERROR;
        }

        lc->slot = slt;
    }

    if ( lc->parent == NULL ) {
        lc->parent = lc->slot->root;
        lc->height = 1;
        if ( lc->parent == NULL ) {
            lc->node = NULL;
            lc->usref = NULL;
            lc->sref  = NULL;
            return DICT_NO_ERROR;
        }
    }

    node *n = lc->parent;
    while ( n != NULL && n != RBLD ) {
        int dir = d->methods->cmp( lc->set->settings->meta, key, n->key );
        switch( dir ) {
            case 0:
                lc->node = n;
                lc->usref = lc->node->usref; // This is never NULL
                lc->sref  = lc->node->usref->sref;

                // If the node has a rebuild value we do not want to use it.
                // But we check after setting it to avoid a race condition.
                // We also use a memory barrier to make sure the set occurs
                // before the check.
                __sync_synchronize();
                if ( lc->sref == RBLD ) {
                    lc->sref = NULL;
                }

                return DICT_NO_ERROR;
            break;
            case -1:
                lc->height++;
                n = n->left;
            break;
            case 1:
                lc->height++;
                n = n->right;
            break;
            default:
                return DICT_API_ERROR;
            break;
        }

        // If the node has no value it has been deref'd
        if ( n != NULL ) {
            lc->parent = n;
        }
    }

    lc->sref = NULL;
    return DICT_NO_ERROR;
}


