#include <unistd.h>
#include <string.h>

#include "devtools.h"
#include "structure.h"
#include "epoch.h"
#include "alloc.h"
#include <stdio.h>

void x_dispose( dict *d, trash *garbage, char *fn, size_t ln ) {
    if ( garbage == NULL ) return;

#ifdef TRASH_CHECK
    if ( garbage->fn ) {
        fprintf( stderr, "\n********\nDouble dispose: %s:%zi - %s,%zi\n*******\n", fn, ln, garbage->fn, garbage->ln );
        dev_assert( 0 );
    }
    garbage->fn = fn;
    garbage->ln = ln;
#endif

    epoch *e = join_epoch( d );

    // Add the trash to the pile
    int success = 0;
    trash *first = NULL;
    while ( !success ) {
        first = e->trash;
        garbage->next = first;
        success = __sync_bool_compare_and_swap( &(e->trash), first, garbage );
    }

    // Try to find a new epoch and put it in place, unless epoch has already
    // changed, or this is the first and only garbage so far.
    if ( first ) {
        advance_epoch( d, e );
    }

    leave_epoch( d, e );
}

epoch *join_epoch( dict *d ) {
    int success = 0;
    epoch *e = NULL;

    while ( !success ) {
        e = &(d->epochs[d->epoch]);
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
        wait_list *w = e->wait_list;

        // This is safe, if nactive is 1 it means no others are active on this
        // epoch, and none will ever join
        e->trash = NULL;
        e->dep   = NULL;
        e->wait_list = NULL;

        // re-open epoch, including memory barrier
        // We want to be sure the epoch is made available before we free the
        // garbage and dependancies in case it is expensive to free them, we do
        // not want the other threads to spin.
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );

        // Free Garbage
        if ( garb != NULL ) free_trash( d, garb );

        // dec dep
        if ( dep != NULL ) leave_epoch( d, dep );

        while ( w != NULL ) {
            wait_list *goner = w;
            w = goner->next;
            // Use atomic swap for memory barrier.
            __sync_bool_compare_and_swap( &(goner->marker), 0, 1 );
        }
    }
}

epoch *wait_epoch( dict *d ) {
    epoch *e = join_epoch( d );

    wait_list w = { NULL, 0 };

    int success = 0;
    while ( !success ) {
        wait_list *next = e->wait_list;
        w.next = next;
        w.marker = 0;
        success = __sync_bool_compare_and_swap( &(e->wait_list), next, &w );
    }

    while ( !advance_epoch( d, e )) sleep(0);
    leave_epoch( d, e );
    while ( !w.marker ) sleep(0);

    return join_epoch( d );
}

int advance_epoch( dict *d, epoch *e ) {
    // If we have a dep, epoch has already been changed.
    if ( e->dep ) return -1;

    uint8_t ci = d->epoch;
    epoch  *ce = &(d->epochs[ci]);

    // Epoch has already been advanced
    if ( ce != e ) return -1;

    for ( uint8_t i = 0; i < EPOCH_LIMIT; i++ ) {
        // Skip current epoch
        if ( i == ci ) continue;
        if ( d->epochs[i].active ) continue;

        if (!__sync_bool_compare_and_swap( &(d->epochs[i].active), 0, 2 )) continue;

        // **** At this point we have a next epoch, and it is active ****

        // Try to create the dependancy relationship, otherwise leave the next
        // epoch.
        if (!__sync_bool_compare_and_swap( &(e->dep), NULL, &(d->epochs[i]) )) {
            // Ooops, something already set a dep...
            leave_epoch( d, &(d->epochs[i]) );
            return -1;
        }

        // **** At this point we have a next epoch, and the dep relationship ****
        dev_assert_or_do( __sync_bool_compare_and_swap( &(d->epoch), ci, i ));

#ifdef METRICS
        __sync_add_and_fetch( &(d->epoch_changed), 1 );
#endif
        return 1;
    }

    // Could not advance epoch, check if another thread has.
    int out = ci == d->epoch ? 0 : -1;

#ifdef METRICS
    if ( out == 0 ) __sync_add_and_fetch( &(d->epoch_failed), 1 );
#endif

    return out;
}
