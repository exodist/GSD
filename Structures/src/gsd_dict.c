#include "gsd_dict_api.h"
#include "gsd_dict_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

const int XRBLD = 1;
const void *RBLD = &XRBLD;

/*\ == TODO ==
 * Replace all occurences of free(loc) and free(locate) with free_location()
 * Add a function for pushing pointers to the epoch stack
 * Update the rem hooks
 * Flesh the collection function
 * Write the rebalance function
 * Write the rebuild function
 * Insert calls to dict_new_epoch() (rebalance, rebuild, do_set, ref and deref)
\*/


// -- Creation and meta data --

void *dict_meta( dict *d ) {
    return d->set->meta;
}

int dict_free( dict **d ) {
    return DICT_UNIMP_ERROR;
}

int dict_create_vb( dict **d, size_t s, size_t mi, void *mta, dict_methods *mth, char *f, size_t l ) {
    if ( mth == NULL ) {
        fprintf( stderr, "Methods may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }
    if ( mth->cmp == NULL ) {
        fprintf( stderr, "The 'cmp' method may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }
    if ( mth->loc == NULL ) {
        fprintf( stderr, "The 'loc' method may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }

    return dict_do_create( d, s, mi, mta, mth );
}

int dict_create( dict **d, size_t s, size_t mi, void *mta, dict_methods *mth ) {
    if ( mth == NULL )      return DICT_API_ERROR;
    if ( mth->cmp == NULL ) return DICT_API_ERROR;
    if ( mth->loc == NULL ) return DICT_API_ERROR;

    return dict_do_create( d, s, mi, mta, mth );
}

// -- Operation --

// Chance to handle pathological data gracefully
int dict_rebuild( dict *d, size_t slots, size_t max_imb, void *meta ) {
    return DICT_UNIMP_ERROR;
}

int dict_get( dict *d, void *key, void **val ) {
    location *loc = NULL;
    int err = dict_locate( d, key, &loc );

    if ( !err ) {
        if ( loc->item == NULL ) {
            *val = NULL;
        }
        else {
            *val = loc->item->value;
        }
    }

    // Free our locator
    if ( loc != NULL ) free( loc );

    return err;
}

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator ) {
    // If these get created we want to hold on to them until the last iteration
    // in case they are needed instead of building them each loop.
    // As such they need to be freed anywhere that returns without referencign
    // them anywhere.
    node *new_node = NULL;
    ref  *new_ref  = NULL;

    while( 1 ) {
        int err = dict_locate( d, key, locator );
        if ( err ) {
            if ( new_node != NULL ) free( new_node );
            if ( new_ref != NULL )  free( new_ref );
            return err;
        }
        location *loc = *locator;

        // Existing ref, safe to update even in a rebuild
        if ( loc->item != NULL ) {
            // We will not need new_node or new_ref anymore.
            if ( new_node != NULL ) free( new_node );
            if ( new_ref != NULL )  free( new_ref );

            if ( loc->item->value != NULL && !override )
                return DICT_EXIST_ERROR;

            int success = 0;

            // If we have old_val it means we only want to place the new value
            // if the old value is what we expect.
            if ( old_val != NULL ) {
                success = __sync_bool_compare_and_swap( &(loc->item->value), old_val, val );
                return success ? 0 : DICT_TRANS_ERROR;
            }

            // Replace the current value, use an atomic swap to ensure we free
            // the value we remove.
            void *ov;
            while ( !success ) {
                ov = loc->item->value;
                success = __sync_bool_compare_and_swap( &(loc->item->value), ov, val );
            }

            // FIXME: this needs to be updated to be a hook
            if ( d->methods->rem != NULL ) d->methods->rem( ov );
            return 0;
        }

        // If we have no item, and cannot create, existance error.
        if ( !create ) {
            if ( new_node != NULL ) free( new_node );
            if ( new_ref != NULL )  free( new_ref );
            return DICT_EXIST_ERROR;
        }

        // We need a new ref
        if ( new_ref == NULL ) {
            new_ref = malloc( sizeof( ref ));
            if ( new_ref == NULL ) return DICT_MEM_ERROR;
            memset( new_ref, 0, sizeof( ref ));
            new_ref->value = val;
            new_ref->refcount = 1;
        }

        // Existing derefed node, lets give it the new ref to revive it
        if ( loc->found != NULL && loc->found != RBLD ) {
            int success = __sync_bool_compare_and_swap( &(loc->found->value), NULL, new_ref );
            if ( success ) {
                if ( new_node != NULL ) free( new_node );
                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );
                return 0;
            }

            // Something else undeleted the node, start over
            if ( loc->found->value != RBLD )
                continue;
        }

        // Node does not exist, we need to create it.
        if ( new_node == NULL ) {
            new_node = malloc( sizeof( node ));
            if ( new_node == NULL ) {
                if ( new_ref != NULL )  free( new_ref  );
                return DICT_MEM_ERROR;
            }
            memset( new_node, 0, sizeof( node ));
            new_node->key = key;
            new_node->value = new_ref;
        }

        // Create slot if necessary
        if ( loc->slt == NULL ) {
            // No slot, and no slot number? something fishy!
            if ( !loc->sltns ) {
                if ( new_node != NULL ) free( new_node );
                if ( new_ref  != NULL ) free( new_ref  );
                return DICT_SLOT_ERROR;
            }

            slot *new_slot = malloc( sizeof( slot ));
            if ( new_slot == NULL ) {
                if ( new_node != NULL ) free( new_node );
                if ( new_ref  != NULL ) free( new_ref  );
                return DICT_MEM_ERROR;
            }
            memset( new_slot, 0, sizeof( slot ));
            new_slot->root = new_node;
            new_slot->node_count = 1;

            // swap into place
            int success = __sync_bool_compare_and_swap(
                &(loc->st->slots[loc->sltn]),
                NULL,
                new_slot
            );

            // If the swap took place we have a new slot, node and ref all in
            // place, job done.
            if ( success ) {
                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );
                return 0;
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
                if ( new_node != NULL ) free( new_node );
                if ( new_ref  != NULL ) free( new_ref  );
                return DICT_CMP_ERROR;
            }

            // If we fail to swap the new node into place, this means another
            // thread beat us to it..
            if ( __sync_bool_compare_and_swap( branch, NULL, new_node )) {
                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key, val );
                return 0;
            }
        }

        // If we found a branch, and its value is not rebuild then another
        // thread beat us to the punch, just start over, if it is RBLD then we
        // are rebuilding
        if ( branch == NULL || *branch != RBLD ) continue;

        // We are in a rebuild, wait for it to end, then we will try again.
        if ( loc->st != NULL && loc->sltns ) {
            // Wait until either the slot rebuild finishes, or a complete
            // rebuild begins.  A complete rebuild could prevent the slot
            // rebuild from finishing.
            while ( d->rebuild == NULL && loc->st->slot_rebuild[loc->sltn] != NULL )
                sleep(0);
        }
        else {
            while ( d->rebuild != NULL ) sleep(0);
        }
    }
}

int dict_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 1, &locator );
    if ( locator != NULL ) {
        if ( !err && locator->st != NULL && locator->imbalance > locator->st->max_imbalance )
            fprintf( stderr, "Imbalance\n" );
        free( locator );
    }
    return err;
}

int dict_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 0, 1, &locator );
    if ( locator != NULL ) {
        if ( !err && locator->st != NULL && locator->imbalance > locator->st->max_imbalance )
            fprintf( stderr, "Imbalance\n" );
        free( locator );
    }
    return err;
}

int dict_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 0, &locator );
    if ( locator != NULL ) free( locator );
    return err;
}

int dict_delete( dict *d, void *key ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, NULL, 1, 0, &locator );
    if ( locator != NULL ) free( locator );
    return err;
}

int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return DICT_API_ERROR;
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) free( locator );
    return err;
}

int dict_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, NULL, 1, 0, &locator );
    if ( locator != NULL ) free( locator );
    return err;
}

int dict_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    //NOTE! if the refcount of the ref we are trying to use is at 0 we cannot raise it.
    return DICT_UNIMP_ERROR;
}

int dict_dereference( dict *d, void *key ) {
    location *loc = NULL;
    int err = dict_locate( d, key, &loc );
    ref *initial = NULL;

    if ( !err && loc != NULL && loc->item != NULL ) {
        initial = loc->item;
    }
    else {
        err = DICT_EXIST_ERROR;
    }

    if ( !err ) {
        // We will need to get a new one
        free( loc );
        loc = NULL;

        // Spin until we can lock out a rebuild of this tree
        while ( !__sync_bool_compare_and_swap( &(loc->found->value), NULL, RBLD )) {
            sleep(0);
        }

        // Find the item again, make sure it matches our initial
        // if it does we will unref it.
        err = dict_locate( d, key, &loc );
        if ( !err && loc->item == initial ) {
            // Remove the ref from the node.
            dict_do_deref( d, &(loc->found->value));
        }

        // Unlock rebuilding this tree.
        if ( !__sync_bool_compare_and_swap( &(loc->found->value), NULL, RBLD )) {
            err = DICT_FATAL_ERROR;
        }
    }

    if ( loc != NULL ) free( loc );
    return err;
}

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    return DICT_UNIMP_ERROR;
}

//----------

void dict_do_deref( dict *d, ref **rr ) {
    ref *r = *rr;
    int success = 0;
    size_t count;
    while ( !success ) {
        count = r->refcount;
        success = __sync_bool_compare_and_swap( &(r->refcount), count, count - 1 );
    }

    if ( count == 0 ) {
        // Nullify rr
        success = 0;
        while ( ! success ) {
            success = __sync_bool_compare_and_swap( rr, *rr, NULL );
        }

        // Release the memory
        // FIXME: this needs to be updated to be a hook
        dict_del *rem = d->methods->rem;
        if ( rem != NULL ) rem( r->value );

        // XXX FIXME: we will use internal garbage collection
        dict_del *del = d->methods->del;
        del( r );
    }
}

int dict_do_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods ) {
    dict *out = malloc( sizeof( dict ));
    if ( out == NULL ) return DICT_MEM_ERROR;
    memset( out, 0, sizeof( dict ));

    out->set = create_set( slots, meta, max_imb );
    if ( out->set == NULL ) {
        free( out );
        return DICT_MEM_ERROR;
    }

    out->methods = methods;

    *d = out;

    return 0;
}

set *create_set( size_t slot_count, void *meta, size_t max_imb ) {
    set *out = malloc( sizeof( set ));
    if ( out == NULL ) return NULL;

    memset( out, 0, sizeof( set ));

    out->max_imbalance = max_imb;
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

/*\
 * epoch count of 0 means free
 * epoch count of 1 means needs to be cleared
 * epoch count of >1 means active
 *
 * Create locator
 * ... stuff ...
 *
    struct epoch {
        size_t active;
        epoch_stack *stack;
    };

    struct epoch_stack {
        epoch_stack *down;
        enum { DSLOT, DNODE, DREF, DSET } type;
        void *to_free;
    };
 *
\*/

void dict_new_epoch( dict *d, epoch *e ) {
    uint8_t oe = d->epoch;

    // Epoch already ended.
    if ( &(d->epochs[oe]) != e ) return;

    uint8_t ne = oe + 1;
    if ( oe >= EPOCH_COUNT ) oe = 0;

    // Bump the epoch, if this fails it just means another thread did it for
    // us, so we ignore the return
    __sync_bool_compare_and_swap( &(d->epoch), oe, ne );
}

void dict_collect( epoch *e ) {
    // Walk the stack and free each item
}

location *dict_create_location( dict *d ) {
    location *locate = malloc( sizeof( location ));
    if ( locate == NULL ) return NULL;
    memset( locate, 0, sizeof( location ));

    epoch *e;

    int success = 0;
    while ( !success ) {
        uint8_t ei = d->epoch;
        epoch *e = &(d->epochs[ei]);

        size_t active = e->active;
        switch (e->active) {
            case 0:
                // Try to set the epoch to 2, thus activating it.
                success = __sync_bool_compare_and_swap( &(e->active), 0, 2 );
            break;

            case 1: // not usable
            break;

            default:
                // Epoch is active, add ourselves to it
                success = __sync_bool_compare_and_swap( &(e->active), active, active + 1 );
            break;
        }
    }

    locate->epoch = e;

    return locate;
}

void dict_free_location( location *locate ) {
    int success = 0;
    size_t nactive;
    while ( !success ) {
        size_t oactive = locate->epoch->active;
        nactive = oactive - 1;
        success = __sync_bool_compare_and_swap( &(locate->epoch->active), oactive, nactive );
    }

    if ( nactive == 1 ) { // we are last, time to clean up.
        // Free Garbage
        dict_collect( locate->epoch );

        // re-open epoch
        __sync_bool_compare_and_swap( &(locate->epoch->active), 1, 0 );
    }

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
        lc->item   = NULL;
    }

    if ( lc->slt == NULL ) {
        slot *slt = lc->st->slots[lc->sltn];

        // Slot is not populated
        if ( slt == NULL ) return 0;
        if ( slt == RBLD ) return 0;

        lc->slt = slt;
    }

    if ( lc->parent == NULL ) {
        lc->parent = lc->slt->root;
        if ( lc->parent == NULL ) return 0;
    }

    node *n = lc->parent;
    while ( n != NULL ) {
        int dir = d->methods->cmp( lc->st->meta, key, n->key );
        switch( dir ) {
            case 0:
                lc->found = n;
                lc->item = lc->found->value;

                // If the node has a rebuild value we do not want to use it.
                // But we check after setting it to avoid a race condition.
                // We also use a memory barrier to make sure the set occurs
                // before the check.
                __sync_synchronize();
                if ( lc->item == RBLD ) {
                    lc->item = NULL;
                }

                return 0;
            break;
            case -1:
                n = n->left;
                if ( n->right == NULL ) lc->imbalance++;
            break;
            case 1:
                n = n->right;
                if ( n->left == NULL ) lc->imbalance++;
            break;
            default:
                return DICT_API_ERROR;
            break;
        }
        lc->parent = n;
    }

    lc->item = NULL;
    return 0;
}

