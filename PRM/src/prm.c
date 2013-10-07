#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>

#include "include/gsd_prm.h"
#include "prm.h"

prm *build_prm( uint8_t epochs, size_t epoch_size, size_t fork_at, destructor *d ) {
    prm *p = malloc(sizeof( prm ));
    if (!p) return NULL;

    p->epochs = malloc(sizeof(epoch) * epochs);
    if (!p->epochs) {
        free(p);
        return NULL;
    }
    memset( p->epochs, 0, sizeof(epoch) * epochs );
    for ( uint8_t i = 0; i < epochs; i++ ) {
        p->epochs[i].dep = -1;
        int ok = new_trash_bag( p, i );
        if (!ok) {
            for ( uint8_t j = 0; j < i; j++ ) {
                free( (trash_bag *)p->epochs[i].trash_bags );
            }
            free( p->epochs );
            free( p );
            return NULL;
        }
    }

    p->detached_threads = 0;

    p->fork_at = fork_at;
    p->destroy = d;
    p->size    = epoch_size;
    p->count   = epochs;
    p->current = 0;

    return p;
}

void free_prm( prm *p ) {
    // wait on detached threads
    // free all trash_bags
    // free epochs
    // free prm
    fprintf( stderr, "Ooops, not implemented, %s line %i\n", __FILE__, __LINE__ );
    abort();
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
    uint8_t dep  = e->dep;

    // This is safe, if nactive is 1 it means no others are active on this
    // epoch, and none will ever join
    e->trash_bags = NULL;
    e->dep        = -1;

    // re-open epoch, including memory barrier
    // We want to be sure the epoch is made available before we free the
    // garbage and dependancies in case it is expensive to free them, we do
    // not want the other threads to spin.
    __sync_bool_compare_and_swap( &(e->active), 1, 0 );

    // Don't spawn a new thread without garbage worth the effort
    size_t count = 0;
    trash_bag *c = b;
    while ( c && count < p->fork_at ) {
        count += c->idx;
        c = c->next;
    }
    if ( count < p->fork_at ) {
        free_garbage( p, b );
        if (dep) leave_epoch( p, dep );
        return;
    }

    __sync_add_and_fetch( &(p->detached_threads), 1 );

    trash_bin *bin  = malloc(sizeof(trash_bin));
    bin->prm        = p;
    bin->trash_bags = b;
    bin->dep        = dep;

    pthread_t pt;
    pthread_create( &pt, NULL, garbage_truck, bin );
    pthread_detach( pt );
}

void *garbage_truck( void *args ) {
    trash_bin *bin = args;

    // Free Garbage
    if ( bin->trash_bags != NULL )
        free_garbage( bin->prm, bin->trash_bags );

    // dec dep
    if ( bin->dep >= 0 )
        leave_epoch( bin->prm, bin->dep );

    __sync_sub_and_fetch( &(bin->prm->detached_threads), 1 );

    free( args );

    return NULL;
}

int dispose( prm *p, void *garbage ) {
    if ( garbage == NULL ) return 1;

    uint8_t e;
    size_t  index;
    while( 1 ) {
        // Get the epoch
        e = p->current;

        // get the index
        trash_bag *b = (trash_bag *)(p->epochs[e].trash_bags);
        index = b->idx;
        if (index < p->size) {
            int ok = __sync_bool_compare_and_swap( &(b->idx), index, index + 1 );
            if (ok) break;
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
        if(!new_trash_bag( p, e )) return 0;

        // the new trashbag will already have idx = 1 so that we can use 0.
        index = 0;
    }

    p->epochs[e].trash_bags->garbage[index] = garbage;

    return 1;
}

int new_trash_bag(prm *p, uint8_t e) {
    trash_bag *new = malloc(sizeof(trash_bag));
    if (!new) return 0;

    new->idx     = 1;
    new->next    = (trash_bag *)(p->epochs[e].trash_bags);
    new->garbage = malloc( sizeof(void *) * p->size );
    if (!new->garbage) {
        free(new);
        return 0;
    }

    assert( __sync_bool_compare_and_swap(&(p->epochs[e].trash_bags), new->next, new) );

    return 1;
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
    
        destructor *d = p->destroy;
        for(size_t i = 0; i < last; i++) {
            if ( d ) {
                d->callback( b->garbage[i], d->arg );
            }
            else {
                free( b->garbage[i] );
            }
        }

        b = b->next;
    }
}
