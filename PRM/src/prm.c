#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>

#include "include/gsd_prm.h"
#include "prm.h"

prm *build_prm( uint8_t epochs, uint8_t epoch_size, size_t thread_at ) {
    if (epoch_size > 64) epoch_size = 64;
    if (epoch_size < 8)  epoch_size = 8;

    prm *p = malloc(sizeof( prm ));
    if (!p) return NULL;
    memset( p, 0, sizeof( prm ));

    p->thread_at = thread_at;
    p->size      = epoch_size;
    p->count     = epochs;

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
        int ok = __atomic_add_fetch( &(p->epochs[i].active), 1, __ATOMIC_SEQ_CST );
        if (ok != 1) {
            __atomic_sub_fetch( &(p->epochs[i].active), 1, __ATOMIC_SEQ_CST);
            for (uint8_t j = 0; j < i; j++) {
                assert( __atomic_sub_fetch( &(p->epochs[j].active), 1, __ATOMIC_SEQ_CST) == 0 );
            }
            return 0;
        }
    }

    __atomic_thread_fence(__ATOMIC_SEQ_CST);

    // Sucks that this needs to be a second loop, but it does for
    // error-handling.
    for (uint8_t i = 0; i < p->count; i++) {
        trash_bag *b = NULL;
        __atomic_load( &(p->epochs[i].trash_bags), &b, __ATOMIC_CONSUME );
        free_garbage( p, b );
    }

    // free epochs
    free( p->epochs );

    // wait on detached threads
    size_t detached = __atomic_load_n( &(p->detached_threads), __ATOMIC_CONSUME );
    while ( detached ) {
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 1 };
        select( 0, NULL, NULL, NULL, &timeout );
        detached = __atomic_load_n( &(p->detached_threads), __ATOMIC_CONSUME );
    }

    free( p );
    return 1;
}

uint8_t join_epoch( prm *p ) {
    int success = 0;
    uint8_t e = 0;

    while ( !success ) {
        e = __atomic_load_n( &(p->current), __ATOMIC_CONSUME );
        size_t active = __atomic_load_n( &(p->epochs[e].active), __ATOMIC_ACQUIRE );
        switch (active) {
            case 0:
                // Try to set the epoch to 2, thus activating it.
                success = __atomic_compare_exchange_n(
                    &(p->epochs[e].active),
                    &active,
                    2,
                    0,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_RELAXED
                );
            break;

            case 1: // not usable, we need to wait for the epoch to change or open.
                sleep( 0 );
            break;

            default:
                // Epoch is active, add ourselves to it
                success = __atomic_compare_exchange_n(
                    &(p->epochs[e].active),
                    &active,
                    active + 1,
                    0,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_RELAXED
                );
            break;
        }
    }

    return e;
}

void leave_epoch( prm *p, uint8_t ei ) {
    epoch *e = p->epochs + ei;
    // ATOMIC __ATOMIC_ACQ_REL
    size_t nactive = __atomic_sub_fetch( &(e->active), 1, __ATOMIC_ACQ_REL );

    if ( nactive != 1 ) return;
    // we are last, time to clean up.

    // Get references to things that need to be cleaned
    trash_bag *b = NULL;
    __atomic_load( &(e->trash_bags), &b, __ATOMIC_ACQUIRE );
    int16_t dep = __atomic_load_n( &(e->dep), __ATOMIC_ACQUIRE );

    // This is safe, if nactive is 1 it means no others are active on this
    // epoch, and none will ever join
    // These do not need to be atomic because of the re-open fence
    e->trash_bags = NULL;
    e->dep        = -1;

    new_trash_bag( p, ei, 0 );

    // re-open epoch, including memory barrier
    // We want to be sure the epoch is made available before we free the
    // garbage and dependancies in case it is expensive to free them, we do
    // not want the other threads to spin.
    assert( __atomic_sub_fetch( &(e->active), 1, __ATOMIC_SEQ_CST ) == 0 );

    // Don't spawn a new thread without garbage worth the effort
    size_t count = 0;
    trash_bag *c = b;
    while ( c && count < p->thread_at ) {
        count += c->idx;
        c = c->next;
    }

    if ( !p->thread_at || count < p->thread_at ) {
        free_garbage( p, b );
    }
    else {
        __atomic_add_fetch( &(p->detached_threads), 1, __ATOMIC_ACQ_REL );

        trash_bin *bin  = malloc(sizeof(trash_bin));
        bin->prm        = p;
        bin->trash_bags = b;
        bin->dep        = dep;

        pthread_t pt;
        pthread_create( &pt, NULL, garbage_truck, bin );
        pthread_detach( pt );
    }

    if (dep != -1) leave_epoch( p, dep );
}

void *garbage_truck( void *args ) {
    trash_bin *bin = args;

    // Free Garbage
    if ( bin->trash_bags != NULL )
        free_garbage( bin->prm, bin->trash_bags );

    __atomic_sub_fetch( &(bin->prm->detached_threads), 1, __ATOMIC_ACQ_REL );

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
        e = __atomic_load_n( &(p->current), __ATOMIC_CONSUME );

        __atomic_load( &(p->epochs[e].trash_bags), &b, __ATOMIC_CONSUME );
        if (!b) {
            new_trash_bag( p, e, 0 );
            continue;
        }

        // get the index
        index = __atomic_load_n( &(b->idx), __ATOMIC_CONSUME );

        if (index < p->size) {
            // We would waste this slot if we get it, cause we need 2 slots,
            // but there is only space for 1.
            int waste = 0;
            if( index + need > p->size) {
                waste = index + need - p->size;
            }

            int ok = __atomic_compare_exchange_n(
                &(b->idx),
                &index,
                index + (waste ? waste : need),
                0,
                __ATOMIC_RELEASE,
                __ATOMIC_RELAXED
            );
            if (ok && !waste) break;

            // NO NEED TO SET WASTE TO NULL, SHOULD ALREADY BE NULL
            continue; // try again :-(
        }

        // Another thread is already responsible for getting things moving
        if ( index > p->size ) continue;

        // index == size, lets see if it is our job to fix things.
        int ok = __atomic_compare_exchange_n(
            &(b->idx),
            &index,
            index + 1,
            0,
            __ATOMIC_RELEASE,
            __ATOMIC_RELAXED
        );
        if (!ok) continue; // Nope!

        // Advance epoch and continue, or we need a new bag
        if( advance_epoch( p, e ) != e ) continue;

        // If this fails then we are out of memory! that sucks!
        b = new_trash_bag( p, e, need );
        if (!b) return 0;

        // the new trashbag will already have idx = need so that we can use 0 -> n-1.
        index = 0;
        break;
    }

    __atomic_store( b->garbage + index, &garbage, __ATOMIC_RELEASE );
    if (destroy) {
        assert( index < 64 );
        uint64_t old = __atomic_load_n( &(b->destructor_map), __ATOMIC_CONSUME );
        while(1) {
            uint64_t new = old | (1 << index);
            if ( arg ) {
                new = new | (1 << (index + 1));
            }

            // ATOMIC __ATOMIC_ACQ_REL/CONSUME
            int ok = __atomic_compare_exchange_n(
                &(b->destructor_map),
                &old,
                new,
                0,
                __ATOMIC_ACQ_REL,
                __ATOMIC_CONSUME
            );
            if (ok) break;
        }

                 __atomic_store( b->garbage + index + 1, &destroy, __ATOMIC_RELEASE );
        if (arg) __atomic_store( b->garbage + index + 2, &arg,     __ATOMIC_RELEASE );
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
    if ( slots ) {
        __atomic_load( &((p->epochs[e].trash_bags)), &(new->next), __ATOMIC_CONSUME );
    }
    else {
        new->next = NULL;
    }

    new->garbage = malloc( sizeof(void *) * p->size );
    if (!new->garbage) {
        free(new);
        return NULL;
    }
    // DO NOT REMOVE THIS, UNUSED GARBAGE MUST BE NULL
    memset( new->garbage, 0, sizeof(void *) * p->size );

    // Clear all bits
    new->destructor_map = 0;

    // ATOMIC ACQ_REL/RELAXED
    int ok = __atomic_compare_exchange(
        &(p->epochs[e].trash_bags),
        &(new->next),
        &new,
        0,
        __ATOMIC_ACQ_REL,
        __ATOMIC_RELAXED
    );
    if (ok) return new;

    free( new->garbage );
    free( new );

    return NULL;
}

uint8_t advance_epoch( prm *p, uint8_t e ) {
    assert(e == p->current);

    for ( uint8_t i = 0; i < p->count; i++ ) {
        // Skip current epoch
        if ( i == e ) continue;
        if ( __atomic_load_n( &(p->epochs[i].active), __ATOMIC_CONSUME )) continue;

        __atomic_store_n( &(p->epochs[i].active), 2, __ATOMIC_RELEASE );
        __atomic_store_n( &(p->epochs[e].dep),    i, __ATOMIC_RELEASE );
        __atomic_store_n( &(p->current),          i, __ATOMIC_SEQ_CST );

        return i;
    }

    return e;
}

void free_garbage( prm *p, trash_bag *b ) {
    __atomic_thread_fence( __ATOMIC_ACQ_REL );
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
