#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#include "include/gsd_prm.h"
#include "prm.h"

prm *build_prm( uint8_t epochs, uint8_t epoch_size, size_t fork_at ) {
    if (epoch_size > 64) epoch_size = 64;
    if (epoch_size < 8)   epoch_size = 8;

    prm *p = malloc(sizeof( prm ));
    if (!p) return NULL;
    memset( p, 0, sizeof( prm ));

    p->fork_at = fork_at;
    p->size    = epoch_size;
    p->count   = epochs;

    p->epochs = malloc(sizeof(epoch) * epochs);
    if (!p->epochs) {
        free(p);
        return NULL;
    }
    memset( p->epochs, 0, sizeof(epoch) * epochs );
    for ( uint8_t i = 0; i < epochs; i++ ) {
        p->epochs[i].dep = -1;
        if (!new_trash_bag( p, i, 0 )) {
            for ( uint8_t j = 0; j < i; j++ ) {
                free( (trash_bag *)p->epochs[i].trash_bags );
            }
            free( p->epochs );
            free( p );
            return NULL;
        }
    }

    return p;
}

int free_prm( prm *p ) {
    // Lock all epochs
    for (uint8_t i = 0; i < p->count; i++) {
        int ok = __sync_bool_compare_and_swap( &(p->epochs[i].active), 0, 1 );
        if (!ok) {
            for (uint8_t j = 0; j < p->count; j++) {
                assert(__sync_bool_compare_and_swap( &(p->epochs[i].active), 1, 0 ));
            }
            return 0;
        }
    }

    // Sucks that this needs to be a second loop, but it does for
    // error-handling.
    for (uint8_t i = 0; i < p->count; i++) {
        free_garbage( p, (trash_bag *)(p->epochs[i].trash_bags) );
    }

    // free epochs
    free( p->epochs );

    // wait on detached threads
    while ( p->detached_threads ) sleep( 0 );

    free( p );
    return 1;
}

uint8_t join_epoch( prm *p ) {
    int success = 0;
    uint8_t e = 0;

    while ( !success ) {
        e = p->current;
        size_t active = p->epochs[e].active;
        switch (active) {
            case 0:
                // Try to set the epoch to 2, thus activating it.
                success = __sync_bool_compare_and_swap( &(p->epochs[e].active), 0, 2 );
            break;

            case 1: // not usable, we need to wait for the epoch to change or open.
                sleep( 0 );
            break;

            default:
                // Epoch is active, add ourselves to it
                success = __sync_bool_compare_and_swap( &(p->epochs[e].active), active, active + 1 );
            break;
        }
    }

    return e;
}

void leave_epoch( prm *p, uint8_t ei ) {
    epoch *e = &(p->epochs[ei]);
    size_t nactive = __sync_sub_and_fetch( &(e->active), 1 );

    if ( nactive != 1 ) return;
    // we are last, time to clean up.

    // Get references to things that need to be cleaned
    trash_bag *b = (trash_bag *)(e->trash_bags);
    int16_t dep  = e->dep;

    // This is safe, if nactive is 1 it means no others are active on this
    // epoch, and none will ever join
    e->trash_bags = NULL;
    e->dep        = -1;

    // re-open epoch, including memory barrier
    // We want to be sure the epoch is made available before we free the
    // garbage and dependancies in case it is expensive to free them, we do
    // not want the other threads to spin.
    if(new_trash_bag( p, ei, 0 )) {
        assert( __sync_bool_compare_and_swap( &(e->active), 1, 0 ));
    }

    // Don't spawn a new thread without garbage worth the effort
    size_t count = 0;
    trash_bag *c = b;
    while ( c && count < p->fork_at ) {
        count += c->idx;
        c = c->next;
    }

    if ( count < p->fork_at ) {
        free_garbage( p, b );
        if (dep != -1) leave_epoch( p, dep );
    }
    else {
        __sync_add_and_fetch( &(p->detached_threads), 1 );

        trash_bin *bin  = malloc(sizeof(trash_bin));
        bin->prm        = p;
        bin->trash_bags = b;
        bin->dep        = dep;

        pthread_t pt;
        pthread_create( &pt, NULL, garbage_truck, bin );
        pthread_detach( pt );
    }

    // If adding a trash bag failed earlier, try again
    // If we fail then we will still open the epoch up the trash bag can be
    // added later.
    if (!e->trash_bags) {
        new_trash_bag( p, ei, 0 );
        assert(__sync_bool_compare_and_swap( &(e->active), 1, 0 ));
    }
}

void *garbage_truck( void *args ) {
    trash_bin *bin = args;

    // Free Garbage
    if ( bin->trash_bags != NULL )
        free_garbage( bin->prm, bin->trash_bags );

    // dec dep
    if ( bin->dep != -1 )
        leave_epoch( bin->prm, bin->dep );

    __sync_sub_and_fetch( &(bin->prm->detached_threads), 1 );

    free( args );

    return NULL;
}

int dispose( prm *p, void *garbage, void (*destroy)(void *, void*), void *arg ) {
    if ( garbage == NULL ) return 1;

    int need = 1;
    if (destroy) {
        need++;
        if (arg) need++;
    }
    else {
        assert( !arg );
    }

    uint8_t e;
    size_t  index;
    trash_bag *b;
    while( 1 ) {
        // Get the epoch
        e = p->current;

        b = (trash_bag *)(p->epochs[e].trash_bags);
        if (!b) {
            new_trash_bag( p, e, 0 );
            continue;
        }

        // get the index
        index = b->idx;

        if (index < p->size) {
            // We would waste this slot if we get it, cause we need 2 slots,
            // but there is only space for 1.
            int waste = 0;
            if( index + need > p->size) {
                waste = index + need - p->size;
            }

            int ok = __sync_bool_compare_and_swap( &(b->idx), index, index + (waste ? waste : need) );
            if (ok && !waste) break;

            for( int i = 0; i < waste; i++ ) {
                p->epochs[e].trash_bags->garbage[index + i] = NULL;
            }
            continue; // try again :-(
        }

        // Another thread is already responsible for getting things moving
        if ( index > p->size ) continue;

        // index == size, lets see if it is out job to fix things.
        int ok = __sync_bool_compare_and_swap( &(b->idx), index, index + 1 );
        if (!ok) continue; // Nope!

        // Try to advance the epoch. 'e' will be correct either way
        e = advance_epoch( p, e );

        // If this fails then we are out of memory! that sucks!
        b = new_trash_bag( p, e, need );
        if (!b) return 0;

        // the new trashbag will already have idx = 1 so that we can use 0.
        index = 0;
        break;
    }

    b->garbage[index] = garbage;
    if (destroy) {
        assert( index < 64 );
        while(1) {
            uint64_t old = b->destructor_map;
            uint64_t new = old | (1 << index);
            if ( arg ) {
                new = new | (1 << (index + 1));
            }
            int ok = __sync_bool_compare_and_swap(
                &(b->destructor_map),
                old,
                new
            );
            if (ok) break;
        }

                 b->garbage[index + 1] = destroy;
        if (arg) b->garbage[index + 2] = arg;
    }

    return 1;
}

trash_bag *new_trash_bag(prm *p, uint8_t e, uint8_t slots) {
    trash_bag *new = malloc(sizeof(trash_bag));
    if (!new) return NULL;
    memset( new, 0, sizeof(trash_bag) );

    // slots will be used by whatever created the bag if this is not the first bag..
    // If no slots are needed this MUST be a first bag, so 'next' must be null
    new->idx  = slots;
    new->next = slots ? (trash_bag *)(p->epochs[e].trash_bags) : NULL;

    new->garbage = malloc( sizeof(void *) * p->size );
    if (!new->garbage) {
        free(new);
        return NULL;
    }
    //memset( new->garbage, 0, sizeof(void *) * p->size );

    // Clear all bits
    new->destructor_map = 0;

    if(__sync_bool_compare_and_swap(&(p->epochs[e].trash_bags), new->next, new)) {
        return new;
    }

    free( new->garbage );
    free( new );

    return NULL;
}

uint8_t advance_epoch( prm *p, uint8_t e ) {
    assert(e == p->current);

    for ( uint8_t i = 0; i < p->count; i++ ) {
        // Skip current epoch
        if ( i == e )              continue;
        if ( p->epochs[i].active ) continue;

        assert( __sync_bool_compare_and_swap( &(p->epochs[i].active), 0, 2 ));
        assert( __sync_bool_compare_and_swap( &(p->epochs[e].dep),   -1, i ));
        assert( __sync_bool_compare_and_swap( &(p->current),          e, i ));

        return i;
    }

    return e;
}

void free_garbage( prm *p, trash_bag *b ) {
    while (b != NULL) {
        size_t last = p->size;
        if (last > b->idx) last = b->idx;

        for(size_t i = 0; i < last; i++) {
            // This happens if a destructor set is added without enough slots
            // remaining
            if ( b->garbage[i] == NULL ) continue;

            int has_d = b->destructor_map & (1 << i);

            if ( has_d ) {
                void (*destroy)(void *, void *) = b->garbage[i+1];
                void *arg = NULL;
                if (b->destructor_map & (1 << (i + 1))) {
                    arg = b->garbage[i+2];
                }

                destroy( b->garbage[i], arg );
                i++; // Make sure we skip the destructor pointer.
                if (arg) i++;
            }
            else {
                free( b->garbage[i] );
            }
        }

        trash_bag *kill = b;
        b = b->next;
        free(kill->garbage);
        free(kill);
    }
}
