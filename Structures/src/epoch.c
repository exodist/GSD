#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "structure.h"
#include "epoch.h"
#include "alloc.h"
#include <stdio.h>

epoch *create_epoch() {
    epoch *new = malloc( sizeof( epoch ));
    if ( new == NULL ) return NULL;
    memset( new, 0, sizeof( epoch ));
    return new;
}

void dispose( dict *d, epoch *e, trash *garbage ) {
    // Add the trash to the pile
    int success = 0;
    while ( !success ) {
        trash *first = e->trash;
        garbage->next = first;
        success = __sync_bool_compare_and_swap( &(e->trash), first, garbage );
    }

    // Try to find a new epoch and put it in place, unless epoch has already changed
    if ( !e->dep ) {
        epoch *last = NULL;
        epoch *next = e->next;
        while ( next != NULL && next != e ) {
            // Start from the beginning if we reach the end, then we will cyle
            // back to e, at which point we abort.
            if ( next->next == NULL ) {
                last = next;
                next = d->epochs;
            }
            else {
                next = next->next;
            }
        }

        if ( next == e && ( d->epoch_count < d->epoch_limit || !d->epoch_limit)) {
            size_t count = __sync_add_and_fetch( &(d->epoch_count), 1 );
            if ( count > d->epoch_limit ) return;
            next = create_epoch();
            if ( next == NULL ) {
                count = __sync_sub_and_fetch( &(d->epoch_count), 1 );
                return;
            }
            assert( __sync_bool_compare_and_swap( &(last->next), NULL, next );
        }

        // Set the next epoch
        if ( next && next != e && __sync_bool_compare_and_swap( &(e->dep), NULL, next )) {
            assert( __sync_bool_compare_and_swap( &(next->active), 0, 2 ));
            assert( __sync_bool_compare_and_swap( &(d->epoch), e, next ));
        }
    }
}

epoch *join_epoch( dict *d ) {
    int success = 0;
    epoch *e = NULL;

    while ( !success ) {
        e = d->epoch;
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

void leave_epoch( dict *d, epoch *e ) {
    size_t nactive = __sync_sub_and_fetch( &(e->active), 1 );

    if ( nactive == 1 ) { // we are last, time to clean up.
        // Get references to things that need to be cleaned
        trash *garb = e->trash;
        epoch *dep  = e->dep;

        // This is safe, if nactive is 1 it means no others are active on this
        // epoch, and none will ever join
        e->trash = NULL;
        e->dep   = NULL;

        // re-open epoch, including memory barrier
        // We want to be sure the epoch is made available before we free the
        // garbage and dependancies in case it is expensive to free them, we do
        // not want the other threads to spin.
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );

        // Free Garbage
        if ( garb != NULL ) free_trash( d, garb );

        // dec dep
        if ( dep != NULL ) leave_epoch( d, dep );
    }
}

