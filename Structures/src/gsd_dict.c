#include "gsd_dict_api.h"
#include "gsd_dict_internal.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

const int XRBLD = 1;
const void *RBLD = &XRBLD;

/* TODO:
 * Split into more files
 * Rebuild
 * Merging
 * Epoch in dot-dump
 * Control max imbalance
\*/

// -- Creation and meta data --

void *dict_get_meta( dict *d ) {
    return d->set->meta;
}

dict_methods *dict_get_methods( dict *d ) {
    return d->methods;
}

int dict_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    // Wait on all epochs
    int active = 1;
    while ( active ) {
        active = 0;
        for ( int i = 0; i < 10; i++ ) {
            epoch *e = &( d->epochs[i] );
            active += e->active;
        }
        if ( active ) sleep( 0 );
    }

    if ( d->set != NULL ) dict_free_set( d, d->set );
    free( d );

    return DICT_NO_ERROR;
}

int dict_create_vb( dict **d, size_t s, void *mta, dict_methods *mth, char *f, size_t l ) {
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

    return dict_do_create( d, s, mta, mth );
}

int dict_create( dict **d, size_t s, void *mta, dict_methods *mth ) {
    if ( mth == NULL )      return DICT_API_ERROR;
    if ( mth->cmp == NULL ) return DICT_API_ERROR;
    if ( mth->loc == NULL ) return DICT_API_ERROR;

    return dict_do_create( d, s, mta, mth );
}

// Copying and cloning
int dict_merge( dict *from, dict *to );
int dict_merge_refs( dict *from, dict *to );

// -- Informational --

int dict_dump_dot( dict *d, char **buffer, dict_dot *show ) {
    epoch *e = NULL;
    dict_join_epoch( d, NULL, &e );
    set *s = d->set;

    dot dt = { NULL, 0, show };

    int error = dict_dump_dot_start( &dt );
    if( error ) {
        dict_leave_epoch( d, e );
        return error;
    }

    int last = -1;
    for ( int i = 0; i < s->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        error = dict_dump_dot_slink( &dt, last, i );
        if ( error ) break;
        last = i;

        node *n = sl->root;
        if ( n == NULL ) continue;
        error = dict_dump_dot_subgraph( &dt, i, n );
        if ( error ) break;
    }

    if ( !error ) {
        error = dict_dump_dot_write( &dt, "}" );
    }

    dict_leave_epoch( d, e );

    *buffer = dt.buffer;

    return error;
}

// -- Operation --

// Chance to handle pathological data gracefully
int dict_rebuild( dict *d, size_t slots, void *meta ) {
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
    if ( loc != NULL ) dict_free_location( d, loc );

    return err;
}

int dict_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 1, &locator );
    if ( locator != NULL ) {
        dict_free_location( d, locator );
    }
    return err;
}

int dict_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 0, 1, &locator );
    if ( locator != NULL ) {
        dict_free_location( d, locator );
    }
    return err;
}

int dict_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_delete( dict *d, void *key ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, NULL, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return DICT_API_ERROR;
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, NULL, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    location *oloc = NULL;
    location *dloc = NULL;

    // Find item in orig, insert if necessary
    int err1 = dict_do_set( orig, okey, NULL, NULL, 0, 1, &oloc );
    // Find item in dest, insert if necessary
    int err2 = dict_do_set( dest, dkey, NULL, NULL, 0, 1, &dloc );

    // Ignore rebalance errors.. might want to readdress this.
    if ( err1 > 100 ) err1 = 0;
    if ( err2 > 100 ) err2 = 0;

    // Transaction error from above simply means it already exists
    if ( err1 == DICT_TRANS_FAIL ) err1 = 0;
    if ( err2 == DICT_TRANS_FAIL ) err2 = 0;

    if ( !err1 && !err2 ) {
        dict_do_deref( dest, dkey, dloc, oloc->itemp->value );
    }

    if ( oloc != NULL ) dict_free_location( orig, oloc );
    if ( dloc != NULL ) dict_free_location( dest, dloc );

    if ( err1 ) return err1;
    if ( err2 ) return err2;

    return DICT_NO_ERROR;
}

int dict_dereference( dict *d, void *key ) {
    location *loc = NULL;
    int err = dict_locate( d, key, &loc );

    if ( !err && loc->item != NULL ) dict_do_deref( d, key, loc, NULL );

    if ( loc != NULL ) dict_free_location( d, loc );
    return err;
}

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    epoch *e = NULL;
    dict_join_epoch( d, NULL, &e );
    set *s = d->set;
    int stop = DICT_NO_ERROR;

    for ( int i = 0; i < s->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        node *n = sl->root;
        if ( n == NULL ) continue;
        stop = dict_iterate_node( d, n, h, args );
        if ( stop ) break;
    }

    dict_leave_epoch( d, e );
    return stop;
}

//----------

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
        // Release the memory
        dict_hook *rem = d->methods->rem;
        if ( rem != NULL ) rem( d, loc->st->meta, key, r->value );

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

void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type ) {
    epoch *effective = e;
    uint8_t effidx;

    while ( 1 ) {
        if (__sync_bool_compare_and_swap( &(effective->garbage), NULL, garbage )) {
            effective->gtype = type;
            effective->meta  = meta;
            // If effective != e, add dep
            if ( e != effective ) {
                e->deps[effidx] = 1;
            }
            return;
        }

        // if the garbage slot is full we need a different epoch
        // if effective is the current epoch, bump the epoch number
        while ( 1 ) {
            uint8_t ei = d->epoch;
            if ( &(d->epochs[ei]) != effective )
                break;

            uint8_t nei = ei + 1;
            if ( nei >= EPOCH_COUNT ) nei = 0;

            if ( __sync_bool_compare_and_swap( &(d->epoch), ei, nei ))
                break;
        }

        // Leave old epoch, unless it is our main epoch
        if ( e != effective )
            dict_leave_epoch( d, effective );

        // Get new effective epoch
        dict_join_epoch( d, &effidx, &effective );
    }
}

void dict_join_epoch( dict *d, uint8_t *idx, epoch **ep ) {
    uint8_t ei;
    epoch *e;

    int success = 0;
    while ( !success ) {
        ei = d->epoch;
        e  = &(d->epochs[ei]);

        size_t active = e->active;
        switch (e->active) {
            case 0:
                // Try to set the epoch to 2, thus activating it.
                success = __sync_bool_compare_and_swap( &(e->active), 0, 2 );
            break;

            case 1: // not usable, we need to wait for the epoch to change or open.
                sleep( 0 );
            break;

            default:
                // Epoch is active, add ourselves to it
                success = __sync_bool_compare_and_swap( &(e->active), active, active + 1 );
            break;
        }
    }

    if ( ep  != NULL ) *ep  = e;
    if ( idx != NULL ) *idx = ei;
}

void dict_leave_epoch( dict *d, epoch *e ) {
    size_t nactive = __sync_sub_and_fetch( &(e->active), 1 );

    if ( nactive == 1 ) { // we are last, time to clean up.
        // Free Garbage
        if ( e->garbage != NULL ) {
            switch ( e->gtype ) {
                case SET:
                    dict_free_set( d, e->garbage );
                break;
                case SLOT:
                    dict_free_slot( d, e->meta, e->garbage );
                break;
                case NODE:
                    dict_free_node( d, e->meta, e->garbage );
                break;
                case SREF:
                    dict_free_sref( d, e->garbage );
                break;
            }
        }

        // This is safe, if nactive is 1 it means no others are active on this
        // epoch, and none will be
        e->garbage = NULL;

        // dec deps
        for ( int i = 0; i < EPOCH_COUNT; i++ ) {
            if ( e->deps[i] ) dict_leave_epoch( d, &(d->epochs[i]) );
            e->deps[i] = 0;
        }

        // re-open epoch
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );
    }
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

size_t tree_ideal_height( size_t count ) {
    size_t ideal = 0;
    while ( count > 0 ) {
        count >>= 1;
        ideal++;
    }

    return ideal;
}

int rebalance( dict *d, location *loc ) {
    // Attempt to create rebalance slot, or return
    slot *ns = malloc( sizeof( slot ));
    if ( ns == NULL ) return DICT_MEM_ERROR;
    memset( ns, 0, sizeof( slot ));

    // Create balance_pair array
    size_t size = loc->slt->count + 100;
    node **all = malloc( sizeof( node * ) * size );
    if ( all == NULL ) {
        free( ns );
        return DICT_MEM_ERROR;
    }
    memset( all, 0, size * sizeof( node * ));

    // Iterate nodes, add to array, block new branches
    size_t count = rebalance_node( loc->slt->root, &all, &size, 0 );
    if ( count == 0 ) return DICT_MEM_ERROR;
    ns->count = count;

    // insert nodes
    size_t ideal = tree_ideal_height( count );
    int ret = rebalance_insert_list( d, loc->st, ns, all, 0, count - 1, ideal );

    // swap
    if (!ret && __sync_bool_compare_and_swap( &(loc->st->slots[loc->sltn]), loc->slt, ns )) {
        dict_dispose( d, loc->epoch, loc->st->meta, loc->slt, SLOT );
    }
    else {
        free( ns );
    }

    free( all );
    return ret;
}

size_t rebalance_node( node *n, node ***all, size_t *size, size_t count ) {
    if ( n->right == NULL ) __sync_bool_compare_and_swap( &(n->right), NULL, RBLD );
    if ( n->right != NULL && n->right != RBLD ) count = rebalance_node( n->right, all, size, count );

    if ( n->value == NULL ) {
        __sync_bool_compare_and_swap( &(n->value), NULL, RBLD );
    }

    if ( n->value != RBLD  ) {
        if ( count >= *size ) {
            node **nall = realloc( *all, (*size + 10) * sizeof(node *));
            *size += 10;
            if ( nall == NULL ) return 0;
            *all = nall;
        }
        (*all)[count] = n;
        count++;
    }

    if ( n->left == NULL ) __sync_bool_compare_and_swap( &(n->left), NULL, RBLD );
    if ( n->left != NULL && n->left != RBLD ) count = rebalance_node( n->left, all, size, count );

    return count;
}

int rebalance_insert( dict *d, set *st, slot *s, node *n, size_t ideal ) {
    size_t height = 0;
    node **put_here = &(s->root);
    while ( *put_here != NULL ) {
        height++;
        int dir = d->methods->cmp( st->meta, n->key, (*put_here)->key );
        if ( dir == -1 ) { // Left
            put_here = &((*put_here)->left);
        }
        else if ( dir == 1 ) { // Right
            put_here = &((*put_here)->right);
        }
        else {
            return DICT_API_ERROR;
        }
    }

    if ( height > ideal + 2 ) return DICT_PATHO_ERROR;

    node *new_node = malloc( sizeof( node ));
    if ( new_node == NULL ) return DICT_MEM_ERROR;
    memset( new_node, 0, sizeof( node ));

    int success = 0;
    while ( !success ) {
        size_t c = n->value->refcount;
        if ( c < 1 ) break;
        success = __sync_bool_compare_and_swap( &(n->value->refcount), c, c + 1 );
    }
    if ( n->value->value == NULL || n->value->value->refcount < 1 ) {
        free( new_node );
        return DICT_NO_ERROR;
    }

    new_node->key   = n->key;
    new_node->value = n->value;

    *put_here = new_node;
    return DICT_NO_ERROR;
}

int rebalance_insert_list( dict *d, set *st, slot *s, node **all, size_t start, size_t end, size_t ideal ) {
    if ( start == end ) return rebalance_insert( d, st, s, all[start], ideal );

    size_t total = end - start;
    size_t half = total / 2;
    size_t center = total % 2 ? start + half + 1
                              : start + half;

    int res = rebalance_insert( d, st, s, all[center], ideal );
    if ( res ) return res;

    res = rebalance_insert_list( d, st, s, all, start, center - 1, ideal );
    if ( res ) return res;

    if ( center == end ) return res;
    res = rebalance_insert_list( d, st, s, all, center + 1, end, ideal );
    return res;
}

int dict_dump_dot_start( dot *dt ) {
    return dict_dump_dot_write( dt,
        "digraph dict {\n    ordering=out\n    bgcolor=black\n    node [color=yellow,fontcolor=white,shape=egg]\n    edge [color=cyan]"
    );
}

int dict_dump_dot_slink( dot *dt, int s1, int s2 ) {
    char buffer[DOT_BUFFER_SIZE];

    // Add the slot node
    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "    s%d [color=green,fontcolor=cyan,shape=box]", s2 );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    // Link it to the last one
    if ( s1 >= 0 ) {
        int ret = snprintf(
            buffer, DOT_BUFFER_SIZE,
            "    s%d->s%d [arrowhead=none,color=yellow]\n    {rank=same; s%d s%d}",
            s1, s2, s1, s2
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        return dict_dump_dot_write( dt, buffer );
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_subgraph( dot *dt, int s, node *n ) {
    char buffer[DOT_BUFFER_SIZE];
    char *label = dt->show( n->key, n->value->value ? n->value->value->value : NULL );
    int ret = 0;

    // Link node
    //s1->10 [arrowhead="none",color=blue]
    ret = snprintf( buffer, DOT_BUFFER_SIZE,
        "    s%d->\"%s\" [arrowhead=none,color=blue]",
        s, label
    );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    // subgraph
//    ret = snprintf( buffer, DOT_BUFFER_SIZE,
//        "    subgraph cluster_s%d {\n        graph [style=dotted,color=grey]",
//        s
//    );
//    if ( ret < 0 ) return DICT_INT_ERROR;
//    ret = dict_dump_dot_write( dt, buffer );
//    if ( ret ) return ret;

    ret = dict_dump_dot_node( dt, buffer, n, label );
    if ( ret ) return ret;

//    return dict_dump_dot_write( dt, "    }" );
    return ret;
}

int dict_dump_dot_node( dot *dt, char *buffer, node *n, char *label ) {
    // This node
    char *style = n->value->value ? n->value->value->value ? ""
                                                           : "[color=pink,fontcolor=pink,style=dashed]"
                                  : "[color=red,fontcolor=red,style=dashed]";

    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\" %s", label, style );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    if ( n->value->value != NULL && n->value->value->refcount > 1 ) {
        int ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        \"%s\"->\"%p\" [color=green,style=dashed]",
            label, n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        \"%p\" [color=white,fontcolor=yellow,shape=hexagon]",
            n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        {rank=sink; \"%p\"}",
            n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    char *left_name  = NULL;
    char *right_name = NULL;
    node *left  = n->left;
    node *right = n->right;
    if ( right != NULL ) {
        right_name = dt->show( right->key, right->value->value ? right->value->value->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }
    else if ( left != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->right), (void *)&(n->right)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    if ( left != NULL ) {
        left_name = dt->show( left->key, left->value->value ? left->value->value->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, left_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }
    else if ( right != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->left), (void *)&(n->left)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    free(label);

    // level
    if ( left_name != NULL && right_name != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        {rank=same; \"%s\" \"%s\"}", left_name, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    // Recurse
    if ( left != NULL ) {
        ret = dict_dump_dot_node( dt, buffer, left, left_name );
        if ( ret ) return ret;
    }
    if ( right != NULL ) {
        ret = dict_dump_dot_node( dt, buffer, right, right_name );
        if ( ret ) return ret;
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_write( dot *dt, char *add ) {
    size_t add_length = strlen( add );

    size_t new = dt->size + add_length + 2;
    char *nb = realloc( dt->buffer, new );
    if ( nb == NULL ) return DICT_MEM_ERROR;
    dt->buffer = nb;

    // First pass
    if ( dt->size == 0 ) dt->buffer[0] = '\0';

    // copy data into buffer
    strncat( dt->buffer, add, new );
    strncat( dt->buffer, "\n", new );
    dt->size = new;

    return DICT_NO_ERROR;
}

