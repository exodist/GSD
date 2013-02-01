#include "gsd_dict_api.h"
#include "gsd_dict_util.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_balance.h"
#include "gsd_dict_location.h"
#include "gsd_dict_free.h"
#include "gsd_dict_epoch.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

size_t tree_ideal_height( size_t count ) {
    size_t ideal = 0;
    while ( count > 0 ) {
        count >>= 1;
        ideal++;
    }

    return ideal;
}

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args ) {
    int stop = 0;

    if ( n->left != NULL && n->left != RBLD ) {
        stop = dict_iterate_node( d, n->left, h, args );
        if ( stop ) return stop;
    }

    usref *ur = n->value;
    sref *sr = ur->value;
    if ( sr != NULL && sr != RBLD ) {
        void *item = sr->value;
        if ( item != NULL ) {
            stop = h( n->key, item, args );
            if ( stop ) return stop;
        }
    }

    if ( n->right != NULL && n->right != RBLD ) {
        stop = dict_iterate_node( d, n->right, h, args );
        if ( stop ) return stop;
    }

    return DICT_NO_ERROR;
}

int dict_do_create( dict **d, size_t slots, void *meta, dict_methods *methods ) {
    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return DICT_MEM_ERROR;
    memset( out, 0, sizeof( dict ));

    out->set = create_set( slots, meta );
    if ( out->set == NULL ) {
        free( out );
        return DICT_MEM_ERROR;
    }

    out->methods = methods;

    *d = out;

    return DICT_NO_ERROR;
}

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator ) {
    // If these get created we want to hold on to them until the last iteration
    // in case they are needed instead of building them each loop.
    // As such they need to be freed anywhere that returns without referencing
    // them anywhere.
    node *new_node = NULL;
    sref *new_sref = NULL;

    while( 1 ) {
        while ( d->rebuild != NULL ) sleep(0);

        int err = dict_locate( d, key, locator );
        if ( err ) {
            if ( new_node != NULL ) dict_free_node( d, d->set, new_node );
            if ( new_sref != NULL ) free( new_sref );
            return err;
        }
        location *loc = *locator;

        // Existing ref, safe to update even in a rebuild
        if ( loc->item != NULL ) {
            // We will not need new_node or new_ref anymore.
            if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
            if ( new_sref != NULL ) free( new_sref );

            if ( loc->item->value != NULL && !override )
                return DICT_TRANS_FAIL;

            int success = 0;

            // If we have old_val it means we only want to place the new value
            // if the old value is what we expect.
            if ( old_val != NULL ) {
                success = __sync_bool_compare_and_swap( &(loc->item->value), old_val, val );
                return success ? 0 : DICT_TRANS_FAIL;
            }

            // Replace the current value, use an atomic swap to ensure we free
            // the value we remove.
            void *ov;
            while ( !success ) {
                ov = loc->item->value;
                success = __sync_bool_compare_and_swap( &(loc->item->value), ov, val );
            }

            if ( d->methods->rem != NULL ) d->methods->rem( d, loc->st->meta, NULL, ov );
            return DICT_NO_ERROR;
        }

        // If we have no item, and cannot create, transaction fail.
        if ( !create ) {
            if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
            if ( new_sref != NULL ) free( new_sref );
            return DICT_TRANS_FAIL;
        }

        // We need a new ref
        if ( new_sref == NULL ) {
            new_sref = malloc( sizeof( sref ));
            if ( new_sref == NULL ) return DICT_MEM_ERROR;
            memset( new_sref, 0, sizeof( sref ));
            new_sref->value = val;
            new_sref->refcount = 1;
        }

        // Existing derefed node, lets give it the new ref to revive it
        if ( loc->found != NULL && loc->found != RBLD ) {
            int success = __sync_bool_compare_and_swap( &(loc->found->value->value), NULL, new_sref );
            if ( success ) {
                if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
                loc->item = new_sref;
                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );
                return DICT_NO_ERROR;
            }

            // Something else undeleted the node, start over
            if ( loc->found->value->value != RBLD )
                continue;
        }

        // Node does not exist, we need to create it.
        if ( new_node == NULL ) {
            new_node = malloc( sizeof( node ));
            if ( new_node == NULL ) {
                if ( new_sref != NULL ) free( new_sref );
                return DICT_MEM_ERROR;
            }
            memset( new_node, 0, sizeof( node ));
            new_node->key = key;
            new_node->value = malloc( sizeof( usref ));
            if ( new_node->value == NULL ) {
                if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
                if ( new_sref != NULL ) free( new_sref );
                return DICT_MEM_ERROR;
            }
            new_node->value->value = new_sref;
        }

        // Create slot if necessary
        if ( loc->slt == NULL ) {
            // No slot, and no slot number? something fishy!
            if ( !loc->sltns ) {
                if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
                if ( new_sref != NULL ) free( new_sref );
                return DICT_INT_ERROR;
            }

            slot *new_slot = malloc( sizeof( slot ));
            if ( new_slot == NULL ) {
                if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
                if ( new_sref != NULL ) free( new_sref );
                return DICT_MEM_ERROR;
            }
            memset( new_slot, 0, sizeof( slot ));
            new_slot->root   = new_node;
            new_slot->count  = 1;
            new_slot->rebuild = 0;

            // swap into place
            int success = __sync_bool_compare_and_swap(
                &(loc->st->slots[loc->sltn]),
                NULL,
                new_slot
            );

            // If the swap took place we have a new slot, node and ref all in
            // place, job done.
            if ( success ) {
                loc->slt = new_slot;
                loc->found = new_node;
                loc->itemp = new_node->value;
                loc->item  = new_node->value->value;

                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );
                return DICT_NO_ERROR;
            }

            // Something else created the slot and set it before we
            // could, free the slot we built :'( then continue.
            free( new_slot );
            continue;
        }

        // We didn't find an existing node, but we did find the nearest parent.
        node **branch = NULL;
        if ( loc->found == NULL && loc->parent != NULL ) {
            // Find the branch to take
            int dir = d->methods->cmp( loc->st->meta, key, loc->parent->key );
            if ( dir == -1 ) { // Left
                branch = &(loc->parent->left);
            }
            else if ( dir == 1 ) { // Right
                branch = &(loc->parent->right);
            }
            else { // This should not be possible.
                if ( new_node != NULL ) dict_free_node( d, loc->st, new_node );
                if ( new_sref != NULL ) free( new_sref );
                return DICT_API_ERROR;
            }

            // If we fail to swap the new node into place, this means another
            // thread beat us to it..
            if ( __sync_bool_compare_and_swap( branch, NULL, new_node )) {
                size_t count = __sync_add_and_fetch( &(loc->slt->count), 1 );

                loc->found = new_node;
                loc->itemp = new_node->value;
                loc->item  = new_node->value->value;

                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );

                size_t height = loc->height + 1;
                size_t ideal  = tree_ideal_height( count );

                // Check if we are balanced with our neighbors
                slot *n1 = loc->st->slots[(loc->sltn + 1) % loc->st->slot_count];
                if (( n1 == NULL && ideal > 2 ) || ( n1 && ideal > 2 + tree_ideal_height( n1->count )))
                    return DICT_PATHO_ERROR;
                slot *n2 = loc->st->slots[(loc->sltn - 1) % loc->st->slot_count];
                if (( n2 == NULL && ideal > 2 ) || ( n2 && ideal > 2 + tree_ideal_height( n2->count )))
                    return DICT_PATHO_ERROR;

                if ( height > ideal + 2 && __sync_bool_compare_and_swap( &(loc->slt->rebuild), 0, 1 )) {
                    fprintf( stderr, "Rebalance\n" );
                    int ret = rebalance( d, loc );
                    loc->slt->rebuild = 0;

                    if ( ret == DICT_PATHO_ERROR )
                        return ret;

                    if ( ret != DICT_NO_ERROR )
                        return DICT_RBAL + ret;
                }

                return DICT_NO_ERROR;
            }

            while ( d->rebuild == NULL && loc->slt->rebuild > 0 )
                sleep(0);
        }
    }
}

void dict_do_deref( dict *d, void *key, location *loc, sref *swap ) {
    sref *r = loc->item;

    // Nullify if the ref is still in the node
    if ( swap != NULL ) {
        int success = 0;
        while ( !success ) {
            size_t sc = swap->refcount;

            // If ref count goes to zero we cannot use it.
            if ( sc == 0 ) {
                swap = NULL;
                break;
            }

            success = __sync_bool_compare_and_swap( &(swap->refcount), sc, sc + 1 );
        }
    }

    __sync_bool_compare_and_swap( &(loc->itemp->value), r, swap );
    size_t count = __sync_sub_and_fetch( &(r->refcount), 1 );

    if ( count == 0 ) {
        dict_dispose( d, loc->epoch, loc->st->meta, r, SREF );
    }
}

set *create_set( size_t slot_count, void *meta ) {
    set *out = malloc( sizeof( set ));
    if ( out == NULL ) return NULL;

    memset( out, 0, sizeof( set ));

    out->meta = meta;
    out->slot_count = slot_count;
    out->slots = malloc( slot_count * sizeof( slot * ));
    if ( out->slots == NULL ) {
        free( out );
        return NULL;
    }

    memset( out->slots, 0, slot_count * sizeof( slot * ));

    return out;
}

