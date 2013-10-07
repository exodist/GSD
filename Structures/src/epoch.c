#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "devtools.h"
#include "structure.h"
#include "epoch.h"
#include "alloc.h"
#include <stdio.h>

epoch_set *build_epoch_set( uint8_t epochs, size_t compactor_size ) {
    epoch_set *s = malloc(sizeof( epoch_set ));
    if (!s) return NULL;

    s->epochs = malloc(sizeof(epoch) * epochs);
    if (!s->epochs) {
        free(s);
        return NULL;
    }

    s->count   = epochs;
    s->current = 0;

    return s;
}

void free_epoch_set( epoch_set *s ) {
    free( s->epochs );
    free( s );
}

epoch *join_epoch( epoch_set *s ) {
    int success = 0;
    epoch *e = NULL;

    while ( !success ) {
        e = &(s->epochs[s->current]);
        size_t active = e->active;
        switch (active) {
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

    return e;
}

void leave_epoch( epoch_set *s, epoch *e ) {
    size_t nactive = __sync_sub_and_fetch( &(e->active), 1 );

    if ( nactive != 1 ) return;
    // we are last, time to clean up.

    // Get references to things that need to be cleaned
    compactor *c = e->compactor;
    epoch *dep   = e->dep;

    // This is safe, if nactive is 1 it means no others are active on this
    // epoch, and none will ever join
    e->compactor = NULL;
    e->dep       = NULL;

    // re-open epoch, including memory barrier
    // We want to be sure the epoch is made available before we free the
    // garbage and dependancies in case it is expensive to free them, we do
    // not want the other threads to spin.
    __sync_bool_compare_and_swap( &(e->active), 1, 0 );

    // Don't spawn a new thread without garbage worth the effort
    if ( c->idx < FORK_FOR_TRASH_COUNT ) {
        free_garbage( s, c );
        if (dep) leave_epoch( s, dep );
        return;
    }

    __sync_add_and_fetch( &(s->detached_threads), 1 );

    garbage_bin *bin = malloc(sizeof(garbage_bin));
    bin->compactor = c;
    bin->dep       = dep;
    bin->set       = s;

    pthread_t p;
    pthread_create( &p, NULL, garbage_truck, bin );
    pthread_detach( p );
}

void *garbage_truck( void *args ) {
    garbage_bin *bin = args;

    // Free Garbage
    if ( bin->compactor != NULL )
        free_garbage( bin->set, bin->compactor );

    // dec dep
    if ( bin->dep != NULL )
        leave_epoch( bin->set, bin->dep );

    free( bin );

    __sync_sub_and_fetch( &(bin->set->detached_threads), 1 );
    return NULL;
}

int dispose( epoch_set *s, void *garbage, destructor *d ) {
    if ( garbage == NULL ) return 1;

    // Add the trash to the pile
    epoch *ne = e;
    while( 1 ) {
        compactor *c = ne->compactor;

        size_t index;
        if (!c) {
            c = new_compactor(e, NULL, s->compactor_size);
            if (!c) {
                if (!ne->compactor) return 0;
                continue;
            }
            // new_compactor sets idx to 1 assuming a slot is needed in the
            // newly created compactor.
            index = 0;
        }
        else {
            index = __sync_fetch_and_add(&(c->idx), 1);

            if ( index >= s->compactor_size ) {
                ne = advance_epoch( s, e );
                if ( ne != e ) continue;

                // Only the first dispose at the end of the compactor should add a
                // new one.
                if ( index != s->compactor_size ) continue;

                // Create a new compactor
                c = new_compactor(e, c, s->compactor_size);
                if (!c) return 0;
                index = 0;
            }
        }

        c->garbage[index].mem     = garbage;
        c->garbage[index].destroy = d;
        return 1;
    }
}

compactor *new_compactor(epoch *e, compactor *current, size_t size) {
    compactor *new = malloc(sizeof(compactor));
    if (!new) return NULL;

    new->idx     = 1;
    new->next    = NULL;
    new->garbage = malloc( sizeof(trash) * size );
    if (!new->garbage) {
        free(new);
        return NULL;
    }

    if (!__sync_bool_compare_and_swap(&(e->compactor), NULL, new)) {
        free(new->garbage);
        free(new);
        return NULL;
    }

    return new;
}

epoch *advance_epoch( epoch_set *s, epoch *e ) {
    // If we have a dep, epoch has already been changed.
    if ( e->dep ) return NULL;

    uint8_t ci = s->current;
    epoch  *ce = &(s->epochs[ci]);

    // Epoch has already been advanced
    if ( ce != e ) return e;

    for ( uint8_t i = 0; i < s->count; i++ ) {
        // Skip current epoch
        if ( i == ci ) continue;
        if ( s->epochs[i].active ) continue;

        if (!__sync_bool_compare_and_swap( &(s->epochs[i].active), 0, 2 )) continue;

        // **** At this point we have a next epoch, and it is active ****

        // Try to create the dependancy relationship, otherwise leave the next
        // epoch.
        if (!__sync_bool_compare_and_swap( &(e->dep), NULL, &(s->epochs[i]) )) {
            // Ooops, something already set a dep...
            leave_epoch( s, &(s->epochs[i]) );
            return e;
        }

        // **** At this point we have a next epoch, and the dep relationship ****
        dev_assert_or_do( __sync_bool_compare_and_swap( &(s->current), ci, i ));

        return &(s->epochs[i]);
    }

    return e;
}

void free_garbage( epoch_set *s, compactor *c ) {
    size_t last = s->compactor_size;
    if (last > c->idx) last = c->idx;

    for(size_t i = 0; i < last; i++) {
        destructor *d = c->garbage[i].destroy;
        if ( d ) {
            d->callback( dc->garbage[i].mem, d->arg );
        }
        else {
            free( dc->garbage[i].mem );
        }
    }
}
