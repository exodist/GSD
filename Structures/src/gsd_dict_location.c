#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_location.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_balance.h"
#include <string.h>

location *dict_create_location( dict *d ) {
    location *locate = malloc( sizeof( location ));
    if ( locate == NULL ) return NULL;
    memset( locate, 0, sizeof( location ));

    dict_join_epoch( d, NULL, &(locate->epoch) );

    return locate;
}

void dict_free_location( dict *d, location *locate ) {
    dict_leave_epoch( d, locate->epoch );
    free( locate );
}

int dict_locate( dict *d, void *key, location **locate ) {
    if ( *locate == NULL ) {
        *locate = dict_create_location( d );
        if ( *locate == NULL ) return DICT_MEM_ERROR;
    }
    location *lc = *locate;

    // The set has been swapped start over.
    if ( lc->st != NULL && lc->st != d->set ) {
        memset( lc, 0, sizeof( location ));
    }

    if ( lc->st == NULL ) {
        lc->st = d->set;
    }

    if ( !lc->sltns ) {
        lc->sltn  = d->methods->loc( lc->st->meta, lc->st->slot_count, key );
        lc->sltns = 1;
    }

    // If the slot has been swapped use the new one (resets decendant values)
    if ( lc->slt != NULL && lc->slt != lc->st->slots[lc->sltn] ) {
        lc->slt    = NULL;
        lc->parent = NULL;
        lc->found  = NULL;
        lc->itemp  = NULL;
        lc->item   = NULL;
    }

    if ( lc->slt == NULL ) {
        slot *slt = lc->st->slots[lc->sltn];

        // Slot is not populated
        if ( slt == NULL || slt == RBLD ){
            lc->parent = NULL;
            lc->found  = NULL;
            lc->itemp  = NULL;
            lc->item   = NULL;
            return DICT_NO_ERROR;
        }

        lc->slt = slt;
    }

    if ( lc->parent == NULL ) {
        lc->parent = lc->slt->root;
        lc->height = 1;
        if ( lc->parent == NULL ) {
            lc->found = NULL;
            lc->itemp = NULL;
            lc->item  = NULL;
            return DICT_NO_ERROR;
        }
    }

    node *n = lc->parent;
    while ( n != NULL && n != RBLD ) {
        int dir = d->methods->cmp( lc->st->meta, key, n->key );
        switch( dir ) {
            case 0:
                lc->found = n;
                lc->itemp = lc->found->value;
                lc->item  = lc->found->value->value;

                // If the node has a rebuild value we do not want to use it.
                // But we check after setting it to avoid a race condition.
                // We also use a memory barrier to make sure the set occurs
                // before the check.
                __sync_synchronize();
                if ( lc->item == RBLD ) {
                    lc->item = NULL;
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

    lc->item = NULL;
    return DICT_NO_ERROR;
}


