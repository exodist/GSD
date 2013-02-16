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

void dispose( dict *d, epoch *e, void *meta, void *garbage, int type ) {
    epoch *end = e;

    while ( 1 ) {
        size_t counter = 0;
        while( end->dep != NULL ) {
            end = end->dep;
            // Check for cycles
            assert( counter++ < 2 + d->epoch_count );
        }

        // Claim the garbage slot, or try again
        if (!__sync_bool_compare_and_swap( &(end->garbage), NULL, garbage )) {
            sleep( 0 );
            continue;
        }

        end->gtype = type;
        end->meta  = meta;

        // Find the first free epoch
        epoch *ready = d->epochs;
        while ( ready->active ) {
            // if there is a next epoch, iterate to it
            if ( ready->next != NULL ) {
                ready = ready->next;
                continue;
            }

            // No next epoch, try to create one
            if ( d->epoch_count < d->epoch_limit || !d->epoch_limit ) {
                epoch *make = create_epoch();
                if ( make != NULL ) {
                    ready->next = make;
                    ready = make;
                    assert(
                        __sync_bool_compare_and_swap(
                            &(d->epoch_count),
                            d->epoch_count,
                            d->epoch_count + 1
                        )
                    );

                    break;
                }
            }

            // Can't create a new epoch, start search over
            ready = d->epochs;
        }

        ready->active = 2;
        assert( __sync_bool_compare_and_swap( &(end->dep), NULL, ready ));
        assert( __sync_bool_compare_and_swap( &(d->epoch), end, ready ));
        return;
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
        void *garb = e->garbage;
        int  gtype = e->gtype;
        epoch *dep = e->dep;

        // This is safe, if nactive is 1 it means no others are active on this
        // epoch, and none will ever join
        e->garbage = NULL;
        e->dep = NULL;

        // re-open epoch, including memory barrier
        // We want to be sure the epoch is made available before we free the
        // garbage and dependancies in case it is expensive to free them, we do
        // not want the other threads to spin.
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );

        // Free Garbage
        if ( garb != NULL ) {
            switch ( gtype ) {
                case SET:
                    free_set( d, garb );
                break;
                case SLOT:
                    free_slot( d, e->meta, garb );
                break;
                case NODE:
                    free_node( d, e->meta, garb );
                break;
                case SREF:
                    free_sref( d, e->meta, garb );
                break;
                case REF:
                    d->methods->ref( d, e->meta, garb, -1 );
                break;
            }
        }

        // dec dep
        if ( dep != NULL ) leave_epoch( d, dep );
    }
}

