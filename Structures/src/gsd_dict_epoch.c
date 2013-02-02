#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_free.h"
#include <unistd.h>

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
                    dict_free_sref( d, e->meta, e->garbage );
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

