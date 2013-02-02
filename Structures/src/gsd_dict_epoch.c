#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_free.h"
#include <unistd.h>
#include <string.h>

epoch **dict_create_epochs( uint8_t epoch_count ) {
    epoch **new = malloc( sizeof( epoch * ) * epoch_count );
    if ( new == NULL ) return NULL;

    // Add padding at the end for the dependancy flags
    int depslots = epoch_count / sizeof( uint8_t )
                 + epoch_count % sizeof( uint8_t ) ? 1 : 0;
    uint8_t padding = sizeof( uint8_t ) * depslots;

    for ( int i = 0; i < epoch_count; i++ ) {
        new[i] = malloc( sizeof( epoch ) + padding );
        if ( new[i] == NULL ) {
            for ( int j = 0; j < i; j++ ) {
                free( new[i] );
            }
            free( new );
            return NULL;
        }
        memset( new[i], 0, sizeof( epoch ) + padding );

        // Point deps at the memory just after the end of our struct which we
        // allocated to be our flags
        new[i]->deps = (void *)(&(new[i]->deps) + 1);
    }

    return new;
}

void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type ) {
    epoch *effective = e;
    uint8_t effidx;

    while ( 1 ) {
        if (__sync_bool_compare_and_swap( &(effective->garbage), NULL, garbage )) {
            effective->gtype = type;
            effective->meta  = meta;
            // If effective != e, add dep
            if ( effective != e ) {
                e->deps[effidx / 8] |= 1 << (effidx % 8);
            }
            return;
        }

        // if the garbage slot is full we need a different epoch
        // if effective is the current epoch, bump the epoch number, if it is
        // not the effective it means somethign else already bumped the epoch
        // for us.
        while ( 1 ) {
            uint8_t ei = d->epoch;

            if ( d->epochs[effidx] != effective )
                break;

            uint8_t nei = ei + 1;
            if ( nei >= d->epoch_count ) nei = 0;

            if ( __sync_bool_compare_and_swap( &(d->epoch), ei, nei ))
                break;
        }

        // Leave old epoch, unless it is our main epoch
        if ( e != effective )
            dict_leave_epoch( d, effective );

        // Get new effective epoch
        dict_join_epoch( d, &effidx, &effective );

        // See if we double-joined the main epoch..
        if ( effective == e ) {
            dict_leave_epoch( d, effective );
        }
    }
}

void dict_join_epoch( dict *d, uint8_t *idx, epoch **ep ) {
    uint8_t ei;
    epoch *e;

    int success = 0;
    while ( !success ) {
        ei = d->epoch;
        e  = d->epochs[ei];

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
        for ( uint8_t i = 0; i < d->epoch_count; i++ ) {
            uint8_t depslot = i / 8;
            uint8_t bitmask = 1 << (i % 8);
            if ( e->deps[depslot] & bitmask ) {
                dict_leave_epoch( d, d->epochs[i] );
                e->deps[depslot] ^= bitmask;
            }
        }

        // re-open epoch
        __sync_bool_compare_and_swap( &(e->active), 1, 0 );
    }
}

