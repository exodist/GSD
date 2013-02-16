#include "string.h"
#include "unistd.h"
#include "stdio.h"

#include "operations.h"
#include "structure.h"
#include "balance.h"
#include "alloc.h"
#include "util.h"

rstat op_get( dict *d, void *key, void **val ) {
    location *loc = NULL;
    rstat err = locate_key( d, key, &loc );

    if ( !err.num ) {
        if ( loc->sref == NULL ) {
            *val = NULL;
        }
        else {
            *val = loc->sref->value;
            if ( d->methods->ref )
                d->methods->ref( d, loc->set->settings->meta, *val, 1 );
        }
    }

    // Free our locator
    if ( loc != NULL ) free_location( d, loc );

    return err;
}

rstat op_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    rstat err = do_set( d, key, NULL, val, 1, 1, &locator );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

rstat op_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    rstat err = do_set( d, key, NULL, val, 0, 1, &locator );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

rstat op_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    rstat err = do_set( d, key, NULL, val, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_delete( dict *d, void *key ) {
    location *locator = NULL;
    rstat err = do_set( d, key, NULL, NULL, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return error( 1, 0, DICT_API_MISUSE, 11 );
    location *locator = NULL;
    rstat err = do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    rstat err = do_set( d, key, old_val, NULL, 1, 0, &locator );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    location *oloc = NULL;
    location *dloc = NULL;

    // Find item in orig, insert if necessary
    rstat err1 = do_set( orig, okey, NULL, NULL, 0, 1, &oloc );
    // Find item in dest, insert if necessary
    rstat err2 = do_set( dest, dkey, NULL, NULL, 0, 1, &dloc );
    rstat ret = rstat_ok;

    // Ignore rebalance errors.. might want to readdress this.
    if ( err1.bit.rebal ) err1 = rstat_ok;
    if ( err2.bit.rebal ) err2 = rstat_ok;

    // Transaction failure from above simply means it already exists
    if ( err1.bit.fail && !err1.bit.error ) err1 = rstat_ok;
    if ( err2.bit.fail && !err1.bit.error ) err2 = rstat_ok;

    if ( !err1.num && !err2.num ) {
        ret = do_deref( dest, dkey, dloc, oloc->usref->sref );
    }

    if ( oloc != NULL ) free_location( orig, oloc );
    if ( dloc != NULL ) free_location( dest, dloc );

    if ( err1.num ) return err1;
    if ( err2.num ) return err2;

    return ret;
}

rstat op_dereference( dict *d, void *key ) {
    location *loc = NULL;
    rstat err = locate_key( d, key, &loc );

    if ( !err.num && loc->sref != NULL ) err = do_deref( d, key, loc, NULL );

    if ( loc != NULL ) free_location( d, loc );
    return err;
}

rstat do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator ) {
    // If these get created we want to hold on to them until the last iteration
    // in case they are needed instead of building them each loop.
    // As such they need to be freed anywhere that returns without referencing
    // them anywhere.
    node *new_node = NULL;
    sref *new_sref = NULL;
    location *loc;

    while( 1 ) {
        rstat err = locate_key( d, key, locator );
        if ( err.num ) {
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

            // If there is a value already, and we can't override, transaction
            // cannot occur.
            if ( loc->sref->value != NULL && !override )
                return rstat_trans;

            if ( d->methods->ref && val )
                d->methods->ref( d, loc->set->settings->meta, val, 1 );

            int success = 0;
            void *ov = old_val;
            if ( ov == NULL ) {
                // Replace the current value, use an atomic swap to ensure we
                // update the ref count of the value we remove.
                while ( !success ) {
                    ov = loc->sref->value;
                    success = __sync_bool_compare_and_swap( &(loc->sref->value), ov, val );
                }
            }
            else {
                // If we have 'old_val' it means we only want to place the new
                // value if the old_value is what we expect
                success = __sync_bool_compare_and_swap( &(loc->sref->value), old_val, val );
                if ( !success ) {
                    if ( d->methods->ref && val )
                        dispose( d, loc->epoch, loc->set->settings->meta, ov, REF );

                    return rstat_trans;
                }
            }

            if ( d->methods->change )
                d->methods->change( d, loc->set->settings->meta, key, ov, val );

            if ( d->methods->ref && ov )
                dispose( d, loc->epoch, loc->set->settings->meta, ov, REF );

            return rstat_ok;
        }

        // If we have no item, and cannot create, transaction fail.
        if ( !create ) {
            if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
            if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
            return rstat_trans;
        }

        // We need a new sref
        if ( new_sref == NULL ) {
            new_sref = malloc( sizeof( sref ));
            if ( new_sref == NULL ) return rstat_mem;
            memset( new_sref, 0, sizeof( sref ));
            new_sref->value = val;
            new_sref->refcount = 1;

            if ( d->methods->ref && val && val != RBLD )
                d->methods->ref( d, loc->set->settings->meta, val, 1 );
        }

        // Existing derefed node, lets give it the new ref to revive it
        if ( loc->node != NULL && loc->node != RBLD ) {
            int success = __sync_bool_compare_and_swap( &(loc->node->usref->sref), NULL, new_sref );
            if ( success ) {
                if ( new_node != NULL ) free_node( d, loc->set->settings->meta, new_node );
                loc->sref = new_sref;

                if ( d->methods->change )
                    d->methods->change( d, loc->set->settings->meta, key, NULL, val );

                return rstat_ok;
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
                return rstat_mem;
            }
            memset( new_node, 0, sizeof( node ));
            new_node->key = key;

            if ( d->methods->ref ) d->methods->ref( d, loc->set->settings->meta, key, 1 );

            new_node->usref = malloc( sizeof( usref ));
            if ( new_node->usref == NULL ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return rstat_mem;
            }
            new_node->usref->refcount = 1;
            new_node->usref->sref = new_sref;
        }

        // Create slot if necessary
        if ( loc->slot == NULL ) {
            // No slot, and no slot number? something fishy!
            if ( !loc->slotn_set ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return error( 1, 0, DICT_UNKNOWN, 4 );
            }

            slot *new_slot = malloc( sizeof( slot ));
            if ( new_slot == NULL ) {
                if ( new_node != NULL ) free_node( d, loc->set, new_node );
                if ( new_sref != NULL ) free_sref( d, loc->set->settings->meta, new_sref );
                return rstat_mem;
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

                if ( d->methods->change )
                    d->methods->change( d, loc->set->settings->meta, key, NULL, val );

                return rstat_ok;
            }

            // Something else created the slot and set it before we
            // could, free the slot we built :'( then continue.
            free( new_slot );
            continue;
        }

        if ( loc->parent == RBLD ) {
            sleep( 0 );
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
                return error( 1, 0, DICT_API_MISUSE, 10 );
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

                if ( d->methods->change )
                    d->methods->change( d, loc->set->settings->meta, key, NULL, val );

                // We add 1 to represent the new node, location does not do it
                // for us.
                size_t  height  = loc->height + 1;
                uint8_t max_imb = loc->set->settings->max_imbalance;

                if ( loc->slot->patho ) {
                    return rstat_ok;
                }
                // Check if we have an internal imbalance
                if ( height > ideal + max_imb && __sync_bool_compare_and_swap( &(loc->slot->rebuild), 0, 1 )) {
                    rstat ret = rebalance( d, loc );
                    __sync_bool_compare_and_swap( &(loc->slot->rebuild), 1, 0 );

                    if ( loc->slot->patho ) {
                        return rstat_patho;
                    }

                    if ( ret.num ) {
                        ret.bit.fail = 0;
                        ret.bit.rebal = 1;
                        return ret;
                    }
                }

                //// Check if we are balanced with our neighbors
                slot *n1 = loc->set->slots[(loc->slotn + 1) % loc->set->settings->slot_count];
                slot *n2 = loc->set->slots[(loc->slotn - 1) % loc->set->settings->slot_count];
                if (( n1 == NULL && ideal > 50 ) || ( n1 && ideal > 50 + n1->ideal_height )) {
                    printf( "\nPatho: %zi+\n", loc->slotn );
                    return rstat_patho;
                }
                if (( n2 == NULL && ideal > 50 ) || ( n2 && ideal > 50 + n2->ideal_height )) {
                    printf( "\nPatho: %zi-\n", loc->slotn );
                    return rstat_patho;
                }

                return rstat_ok;
            }

            while ( loc->slot->rebuild > 0 ) sleep(0);
        }
    }
}

rstat do_deref( dict *d, void *key, location *loc, sref *swap ) {
    sref *r = loc->sref;
    if ( r == NULL ) return rstat_trans;

    if ( swap != NULL ) {
        int success = 0;
        while ( !success ) {
            size_t sc = swap->refcount;

            // If ref count goes to zero we cannot use it.
            if ( sc == 0 ) return rstat_trans;

            success = __sync_bool_compare_and_swap( &(swap->refcount), sc, sc + 1 );
        }
    }

    // Swap old sref with new sref, skip if sref has changed already
    __sync_bool_compare_and_swap( &(loc->usref->sref), r, swap );

    if ( d->methods->change )
        d->methods->change( d, loc->set->settings->meta, key, r->value, swap ? swap->value : NULL );

    // Lower ref count of old sref, dispose of sref if count hits 0
    size_t count = __sync_sub_and_fetch( &(r->refcount), 1 );
    if ( count == 0 ) {
        dispose( d, loc->epoch, loc->set->settings->meta, r, SREF );
    }

    return rstat_ok;
}
