#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_free.h"
#include <unistd.h>
#include <string.h>

epoch *dict_create_epoch() {
    epoch *new = malloc( sizeof( epoch ));
    if ( new == NULL ) return NULL;
    memset( new, 0, sizeof( epoch ));
    return new;
}

void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type ) {
    epoch *end = e;
    
    while ( 1 ) {
        while( end->dep != NULL ) end = end->dep;

        // Claim the garbage slot, or try again
        if (!__sync_bool_compare_and_swap( &(end->garbage), NULL, garbage )) {
            sleep( 0 );
            continue;
        }

        end->gtype = type;
        end->meta  = meta;

        // Find the first free epoch
        epoch *new = d->epochs;
        while ( new->active ) {
            // if there is a next epoch, iterate to it
            if ( new->next != NULL ) {
                new = new->next;
                continue;
            }

            // No next epoch, try to create one
            if ( d->epoch_count < d->epoch_limit || !d->epoch_limit ) {
                epoch *make = dict_create_epoch();
                if ( make != NULL ) {
                    new->next = make;
                    new = make;
                    break;
                }
            }

            // Can't create a new epoch, start search over
            new = d->epochs;
        }

        end->dep = new;
        new->active = 2;
        __sync_bool_compare_and_swap( &(d->epoch), end, new );
        return;
    }
}

epoch *dict_join_epoch( dict *d ) {
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
                    dict_free_sref( d, e->meta, e->garbage );
                break;
            }
        }

        // This is safe, if nactive is 1 it means no others are active on this
        // epoch, and none will be
        e->garbage = NULL;

        // dec dep
        if ( e->dep != NULL ) {
            dict_leave_epoch( d, e->dep );
            e->dep = NULL;
        }

        // re-open epoch
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );
    }
}

