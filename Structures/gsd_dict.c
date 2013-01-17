#include "gsd_dict.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct slot slot;
typedef struct node node;
typedef struct ref  ref;
typedef struct set  set;

typedef struct location location;

struct dict {
    set *set;
    set *rebuild;

    dict_methods *methods;
};

struct set {
    slot  **slots;
    slot  **slot_rebuild;
    size_t  slot_count;
    size_t  max_imbalance;
    void   *meta;
};

struct slot {
    node   *root;
    size_t node_count;
    size_t deleted;
};

struct node {
    node *left;
    node *right;
    void *key;
    ref  *value;
};

struct ref {
    size_t  refcount;
    void   *value;
};

struct location {
    set    *st;
    size_t  sltn;
    uint8_t sltns;
    slot   *slt;
    node   *parent;
    node   *found;
    ref    *item;
    size_t imbalance;
};

set *create_set( size_t slot_count, void *meta, size_t max_imb );

const int XRBLD = 1;
const void *RBLD = &XRBLD;

// success = __sync_bool_compare_and_swap( item, oldval, newval );

// -- Creation and meta data --

void *dict_meta( dict *d ) {
    return d->set->meta;
}

int x_dict_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods, char *file, size_t line ) {
    if ( methods == NULL ) {
        fprintf( stderr, "Methods may not be NULL. Called from %s line %zi", file, line );
        return DICT_API_ERROR;
    }
    if ( methods->cmp == NULL ) {
        fprintf( stderr, "The 'cmp' method may not be NULL. Called from %s line %zi", file, line );
        return DICT_API_ERROR;
    }
    if ( methods->loc == NULL ) {
        fprintf( stderr, "The 'loc' method may not be NULL. Called from %s line %zi", file, line );
        return DICT_API_ERROR;
    }
    if ( methods->del == NULL ) {
        fprintf( stderr, "The 'del' method may not be NULL. Called from %s line %zi", file, line );
        return DICT_API_ERROR;
    }

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

// -- Operation --

// Chance to handle pathological data gracefully
int dict_rebuild( dict *d, size_t slots, size_t max_imb, void *meta ) {
    return DICT_UNIMP_ERROR;
}

int dict_locate( dict *d, void *key, location **locate ) {
    location *lc = *locate;

    if ( lc == NULL ) {
        lc = malloc( sizeof( location ));
        if ( lc == NULL ) return DICT_MEM_ERROR;
        memset( lc, 0, sizeof( location ));
        *locate = lc;
    }

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

            if ( !override ) return DICT_EXIST_ERROR;

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

        // Existing deleted node, lets give it the new ref to undelete it
        if ( loc->found != NULL && loc->found != RBLD ) {
            int success = __sync_bool_compare_and_swap( &(loc->found->value), NULL, new_ref );
            if ( success ) {
                if ( new_node != NULL ) free( new_node );
                if ( d->methods->ins != NULL )
                    d->methods->ins( d, loc->st->meta, key );
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
                    d->methods->ins( d, loc->st->meta, key );
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
                    d->methods->ins( d, loc->st->meta, key );
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

void dict_deref( dict *d, ref *r ) {
    int success = 0;
    size_t count;
    while ( !success ) {
        count = r->refcount;
        success = __sync_bool_compare_and_swap( &(r->refcount), count, count - 1 );
    }

    if ( count == 0 ) {
        dict_del *rem = d->methods->rem;
        dict_del *del = d->methods->del;
        if ( rem != NULL ) rem( r->value );
        del( r );
    }
}

int dict_delete( dict *d, void *key ) {
    int err = 0;
    int go = 1;

    while ( go ) {
        location *loc = NULL;
        err = dict_locate( d, key, &loc );

        // If there is an error, or nothing to delete
        if ( err || loc == NULL || loc->item == NULL ) {
            go = 0;
        }
        else {
            ref *oldval;
            int success = 0;
            while ( !success ) {
                oldval = loc->found->value;
                success = __sync_bool_compare_and_swap( &(loc->found->value), oldval, NULL );
            }

            dict_deref( d, oldval );

            go = 0;

            // A rebuild might have carried over the ref to a new set or slot.
            __sync_synchronize();
            if ( loc->st  != d->set )                        go = 1;
            if ( loc->slt != d->set->slots[loc->sltn] )      go = 1;
            if ( d->rebuild != NULL )                        go = 1;
            if ( loc->st->slot_rebuild[loc->sltn] != NULL )  go = 1;

            if ( go == 1 ) {
                while ( d->rebuild == NULL && loc->st->slot_rebuild[loc->sltn] != NULL )
                    sleep(0);
                while ( d->rebuild != NULL )
                    sleep(0);
            }
            else {
                if ( d->methods->dlt != NULL )
                    d->methods->dlt( d, loc->st->meta, key );
            }
        }

        if ( loc != NULL ) free( loc );
    }

    return err;
}

int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) free( locator );
    return err;
}

int dict_reference( dict *orig, dict *dest, dict *key ) {
    //NOTE! if the refcount of the ref we are trying to use is at 0 we cannot raise it.
    return DICT_UNIMP_ERROR;
}

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    return DICT_UNIMP_ERROR;
}
