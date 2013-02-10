#include "string.h"
#include "unistd.h"

#include "include/gsd_dict_return.h"

#include "operations.h"
#include "structure.h"
#include "balance.h"
#include "alloc.h"
#include "util.h"

//-------------------
// These functions are defined in gsd_dict.h
// They are publicly exposed functions.
// Changing how these work requires a major version bump.
//-------------------

int op_get( dict *d, void *key, void **val ) {
    location *loc = NULL;
    int err = locate_key( d, key, &loc );

    if ( !err ) {
        if ( loc->sref == NULL ) {
            *val = NULL;
        }
        else {
            *val = loc->sref->value;
        }
    }

    // Free our locator
    if ( loc != NULL ) free_location( d, loc );

    return err;
}

int op_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = do_set( d, key, NULL, val, 1, 1, &locator );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

int op_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = do_set( d, key, NULL, val, 0, 1, &locator );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

int op_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = do_set( d, key, NULL, val, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

int op_delete( dict *d, void *key ) {
    location *locator = NULL;
    int err = do_set( d, key, NULL, NULL, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

int op_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return DICT_API_ERROR;
    location *locator = NULL;
    int err = do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

int op_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    int err = do_set( d, key, old_val, NULL, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

int op_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    location *oloc = NULL;
    location *dloc = NULL;

    // Find item in orig, insert if necessary
    int err1 = do_set( orig, okey, NULL, NULL, 0, 1, &oloc );
    // Find item in dest, insert if necessary
    int err2 = do_set( dest, dkey, NULL, NULL, 0, 1, &dloc );

    // Ignore rebalance errors.. might want to readdress this.
    if ( err1 > 100 ) err1 = 0;
    if ( err2 > 100 ) err2 = 0;

    // Transaction error from above simply means it already exists
    if ( err1 == DICT_TRANS_FAIL ) err1 = 0;
    if ( err2 == DICT_TRANS_FAIL ) err2 = 0;

    if ( !err1 && !err2 ) {
        do_deref( dest, dkey, dloc, oloc->usref->sref );
    }

    if ( oloc != NULL ) free_location( orig, oloc );
    if ( dloc != NULL ) free_location( dest, dloc );

    if ( err1 ) return err1;
    if ( err2 ) return err2;

    return DICT_NO_ERROR;
}

int op_dereference( dict *d, void *key ) {
    location *loc = NULL;
    int err = locate_key( d, key, &loc );

    if ( !err && loc->sref != NULL ) err = do_deref( d, key, loc, NULL );

    if ( loc != NULL ) free_location( d, loc );
    return err;
}

//------------------------------------------------
// Nothing below here is publicly exposed.
//------------------------------------------------

int do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator ) {
    // If these get created we want to hold on to them until the last iteration
    // in case they are needed instead of building them each loop.
    // As such they need to be freed anywhere that returns without referencing
    // them anywhere.
    node *new_node = NULL;
    sref *new_sref = NULL;
    location *loc;

    while( 1 ) {
        int err = locate_key( d, key, locator );
        if ( err ) {
            if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
            if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
            return err;
        }
        loc = *locator;

        // Existing sref, safe to update even in a rebuild
        if ( loc->sref != NULL ) {
            // We will not need new_node or new_sref anymore.
            if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
            if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );

            // If there is a value already, cand we can't override, transaction
            // cannot occur.
            if ( loc->sref->value != NULL && !override )
                return DICT_TRANS_FAIL;

            int success = 0;
            if ( old_val == NULL ) {
                // Replace the current value, use an atomic swap to ensure we
                // update the ref count of the value we remove.
                while ( !success ) {
                    old_val = loc->sref->value;
                    success = __sync_bool_compare_and_swap( &(loc->sref->value), old_val, val );
                }
            }
            else {
                // If we have 'old_val' it means we only want to place the new
                // value if the old_value is what we expect
                success = __sync_bool_compare_and_swap( &(loc->sref->value), old_val, val );
                if ( !success ) return DICT_TRANS_FAIL;
            }

            // REF TODO: Trigger Change
            // REF TODO: val gains a ref
            // REF TODO: old_val loses a ref (if not null)

            return DICT_NO_ERROR;
        }

        // If we have no item, and cannot create, transaction fail.
        if ( !create ) {
            if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
            if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
            return DICT_TRANS_FAIL;
        }

        // We need a new sref
        if ( new_sref == NULL ) {
            new_sref = malloc( sizeof( sref ));
            if ( new_sref == NULL ) return DICT_MEM_ERROR;
            memset( new_sref, 0, sizeof( sref ));
            new_sref->value = val;
            new_sref->refcount = 1;
            // REF TODO: val gains a ref
        }

        // Existing derefed node, lets give it the new ref to revive it
        if ( loc->node != NULL && loc->node != RBLD ) {
            int success = __sync_bool_compare_and_swap( &(loc->node->usref->sref), NULL, new_sref );
            if ( success ) {
                if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
                loc->sref = new_sref;

                // REF TODO: Trigger Change

                return DICT_NO_ERROR;
            }

            // Something else undeleted the node, start over
            if ( loc->node->usref->sref != RBLD )
                continue;
        }

        // Node does not exist, we need to create it.
        if ( new_node == NULL ) {
            new_node = malloc( sizeof( node ));
            if ( new_node == NULL ) {
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return DICT_MEM_ERROR;
            }
            memset( new_node, 0, sizeof( node ));
            new_node->key = key;
            // REF TODO: key gains a ref
            new_node->usref = malloc( sizeof( usref ));
            if ( new_node->usref == NULL ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return DICT_MEM_ERROR;
            }
            new_node->usref->sref = new_sref;
        }

        // Create slot if necessary
        if ( loc->slot == NULL ) {
            // No slot, and no slot number? something fishy!
            if ( !loc->slotn_set ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return DICT_INT_ERROR;
            }

            slot *new_slot = malloc( sizeof( slot ));
            if ( new_slot == NULL ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return DICT_MEM_ERROR;
            }
            memset( new_slot, 0, sizeof( slot ));
            new_slot->root   = new_node;
            new_slot->count  = 1;
            new_slot->rebuild = 0;

            // swap into place
            int success = __sync_bool_compare_and_swap(
                &(loc->set->slots[loc->slotn]),
                NULL,
                new_slot
            );

            // If the swap took place we have a new slot, node and ref all in
            // place, job done.
            if ( success ) {
                loc->slot = new_slot;
                loc->node = new_node;
                loc->usref = new_node->usref;
                loc->sref  = new_node->usref->sref;

                // REF TODO: Trigger Change

                return DICT_NO_ERROR;
            }

            // Something else created the slot and set it before we
            // could, free the slot we built :'( then continue.
            free( new_slot );
            continue;
        }

        // We didn't find an existing node, but we did find the nearest parent.
        node **branch = NULL;
        if ( loc->node == NULL && loc->parent != NULL ) {
            // Find the branch to take
            int dir = d->methods->cmp( loc->set->settings, key, loc->parent->key );
            if ( dir == -1 ) { // Left
                branch = &(loc->parent->left);
            }
            else if ( dir == 1 ) { // Right
                branch = &(loc->parent->right);
            }
            else { // This should not be possible.
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return DICT_API_ERROR;
            }

            // If we fail to swap the new node into place, this means another
            // thread beat us to it..
            if ( __sync_bool_compare_and_swap( branch, NULL, new_node )) {
                size_t count = __sync_add_and_fetch( &(loc->slot->count), 1 );
                uint8_t ideal = max_bit( count );

                uint8_t old_ideal = loc->slot->ideal_height;
                while ( ideal > old_ideal && count >= loc->slot->count ) {
                    if( !__sync_bool_compare_and_swap( &(loc->slot->ideal_height), old_ideal, ideal ))
                        old_ideal = loc->slot->ideal_height;
                }

                loc->node = new_node;
                loc->usref = new_node->usref;
                loc->sref  = new_node->usref->sref;

                // REF TODO: Trigger Change

                // We add 1 to represent the new node, location does not do it
                // for us.
                size_t  height  = loc->height + 1;
                uint8_t max_imb = loc->set->settings->max_imbalance;

                // Check if we are balanced with our neighbors
                slot *n1 = loc->set->slots[(loc->slotn + 1) % loc->set->settings->slot_count];
                slot *n2 = loc->set->slots[(loc->slotn - 1) % loc->set->settings->slot_count];
                if (( n1 == NULL && ideal > max_imb ) || ( n1 && ideal > max_imb + n1->ideal_height ))
                    return DICT_PATHO_ERROR;
                if (( n2 == NULL && ideal > max_imb ) || ( n2 && ideal > max_imb + n2->ideal_height ))
                    return DICT_PATHO_ERROR;

                // Check if we have an internal imbalance
                if ( height > ideal + max_imb && __sync_bool_compare_and_swap( &(loc->slot->rebuild), 0, 1 )) {
                    int ret = rebalance( d, loc );
                    loc->slot->rebuild = 0;

                    if ( ret == DICT_PATHO_ERROR )
                        return ret;

                    if ( ret != DICT_NO_ERROR )
                        return DICT_RBAL + ret;
                }

                return DICT_NO_ERROR;
            }

            while ( loc->slot->rebuild > 0 ) sleep(0);
        }
    }
}

int do_deref( dict *d, void *key, location *loc, sref *swap ) {
    sref *r = loc->sref;
    if ( r == NULL ) return DICT_TRANS_FAIL;

    if ( swap != NULL ) {
        int success = 0;
        while ( !success ) {
            size_t sc = swap->refcount;

            // If ref count goes to zero we cannot use it.
            if ( sc == 0 ) return DICT_TRANS_FAIL;

            success = __sync_bool_compare_and_swap( &(swap->refcount), sc, sc + 1 );
        }
    }

    // Swap old sref with new sref, skip if sref has changed already
    __sync_bool_compare_and_swap( &(loc->usref->sref), r, swap );

    // Lower ref count of old sref, dispose of sref if count hits 0
    size_t count = __sync_sub_and_fetch( &(r->refcount), 1 );
    if ( count == 0 ) {
        dispose( d, loc->epoch, loc->set->settings->meta, r, SREF );
    }

    return DICT_NO_ERROR;
}
