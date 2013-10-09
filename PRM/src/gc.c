#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "include/gsd_gc.h"
#include "gc.h"

collector *build_collector(
    // Check if an item is iterable, and how
    gc_iterable *iterable,

    // Iterate items via an iterator object
    gc_iterator     *get_iterator,
    gc_iterate_next *next,

    // Objects that need to iterate for you with a callback
    gc_iterate  *iterate,
    gc_callback *callback,

    gc_destructor *destroy,
    void          *destarg
) {
    // Code later relies on this assertion, but we put it here to keep it from
    // running more than once.
    assert(sizeof(tag) == 8);

    collector *c = malloc(sizeof(collector));
    if (!c) return NULL;

    memset(c, 0, sizeof(collector));

    c->iterable     = iterable;
    c->get_iterator = get_iterator;
    c->next         = next;
    c->iterate      = iterate;
    c->callback     = callback;
    c->destroy      = destroy;
    c->destarg      = destarg;

    return c;
}

void start_collector( collector *c ) {
    if (c->started) return;

    // Make sure we have at least one root
    assert( c->root_size && c->root_index && c->roots[0] );

    // create the pthread and start it
    assert( __sync_bool_compare_and_swap( &(c->started), 0, 1 ));
    assert( !pthread_create( &(c->thread), NULL, collector_thread, c ));
}

void *free_collector( collector *c ) {
    assert( __sync_bool_compare_and_swap( &(c->stopped), 0, 1 ));

    // Make sure no epochs are active
    while ( c->epochs[0] || c->epochs[1] ) sleep(0);

    // wait on pthread
    pthread_join( c->thread, NULL );

    // free buckets
    for (size_t i = 0; i < MAX_BUCKET; i++) {
        bucket *b = (bucket *)(c->buckets[i]);
        while (b) {
            bucket *kill = b;
            b = b->next;
            free(kill);
        }
    }

    // free bigs
    bigtag *bt = (bigtag *)(c->big);
    while (bt) {
        bigtag *kill = bt;
        bt = bt->next;
        free(kill);
    }

    // free roots
    for( size_t i = 0; i < c->root_index; i++ ) {
        c->destroy( c->destarg, c->roots[i] );
    }
    free( c->roots );

    // free collector
    void *arg = c->destarg;
    free(c);
    return arg;
}

void *gc_alloc_root( collector *c, size_t size ) {
    assert( !c->started );

    size_t idx = c->root_index++;
    if ( idx == c->root_size ) {
        void *check = realloc( c->roots, c->root_size + (sizeof(tag *) * 5));
        if (!check) return NULL;
        c->roots = check;
        c->root_size += sizeof(tag *) * 5;
    }

    tag *alloc = malloc(sizeof(tag) + size);
    if (!alloc) {
        c->root_index--;
        return NULL;
    }
    memset( alloc, 0, sizeof(tag) + size );
    alloc->active = 1;
    alloc->bucket = -1;
    alloc->state  = GC_CHECKED;
    c->roots[idx] = alloc;

    // Return data section
    return (void *)(alloc + 1);
}

tag *gc_tag( void *alloc ) {
    tag *t = alloc;
    return t - 1;
}

uint8_t *gc_pad( void *alloc ) {
    return (uint8_t *)(gc_tag(alloc)->pad);
}

int atomic_tag_update( tag *tp, tag oldv, tag newv ) {
    union { tag *t; uint64_t *i; } t;
    union { tag  t; uint64_t  i; } old;
    union { tag  t; uint64_t  i; } new;

    t.t = tp;
    old.t = oldv;
    new.t = newv;

    return __sync_bool_compare_and_swap( t.i, old.i, new.i );
}

void gc_activate ( void *alloc ) {
    tag *t = gc_tag(alloc);

    while (1) {
        tag old = *t;
        tag new = *t;

        new.active++;
        switch( new.state ) {
            case GC_UNCHECKED:
            case GC_TO_CHECK:
            case GC_ACTIVE_TO_CHECK:
                new.state = GC_ACTIVE_TO_CHECK;
                break;

            case GC_CHECKED: break;

            default:
                assert(
                    new.state == GC_UNCHECKED       ||
                    new.state == GC_TO_CHECK        ||
                    new.state == GC_ACTIVE_TO_CHECK ||
                    new.state == GC_CHECKED
                );
                abort();
        }

        // Atomic swap( t, old, new );
        if (atomic_tag_update(t, old, new)) break;
    }
}

void gc_deactivate( void *alloc ) {
    // Much easier since we don't need to change the state at all.
    tag *t = gc_tag(alloc);
    __sync_sub_and_fetch( &(t->active), 1 );
}

uint8_t gc_join_epoch( collector *c ) {
    while(1) {
        uint8_t e  = c->epoch;
        size_t val = c->epochs[e];
        switch( val ) {
            case 0:
                if (__sync_bool_compare_and_swap( c->epochs + e, 0, 2 ))
                    return e;

            case 1: break;

            default:
                if (__sync_bool_compare_and_swap( c->epochs + e, val, val + 1 ))
                    return e;
        }
    }
}

void gc_leave_epoch( collector *c, uint8_t e ) {
    // Much easier to leave then to join :-)
    __sync_sub_and_fetch( c->epochs + e, 1 );
}

void *gc_alloc ( collector *c, size_t size ) {
    // if buckets[size - 1] use bucket
    // else malloc and push onto 'big'
    // mark as CHECKED
    // set bucket ? Maybe it should just always be set?
    // set active
    // 0 out padding
}

unsigned int update_to_unchecked( collector *c, tag *t ) {
    // if active set GC_ACTIVE_TO_CHECK
    // else      set GC_UNCHECKED
}

unsigned int update_to_checked( collector *c, tag *t ) {
    tag *t = gc_tag(alloc);
    while (1) {
        tag old = *t;
        tag new = *t;

        switch( old.state ) {
            case GC_FREE: return 0;

            case GC_TO_CHECK:
            case GC_ACTIVE_TO_CHECK:
                new.state = GC_FREE;
            break;

            default: return 0
        }

        // Atomic swap( t, old, new );
        if (atomic_tag_update(t, old, new)) break;
    }

    //TODO: children
    assert(0);

    return 1;
}

unsigned int update_to_free( collector *c, tag *t ) {
    tag *t = gc_tag(alloc);
    while (1) {
        tag old = *t;
        tag new = *t;

        switch( old.state ) {
            case GC_FREE: return 0;

            case GC_UNCHECKED:
                new.state = GC_FREE;
            break;

            default: return update_to_unchecked( c, t );
        }

        // Atomic swap( t, old, new );
        if (atomic_tag_update(t, old, new)) break;
    }

    // TODO add to free list.
    assert(0);

    return 1;
}

size_t collector_cycle(collector *c, unsigned int (*update)(collector *c, tag *t) ) {
    size_t found = 0;

    for(int i = 0; i < MAX_BUCKET; i++) {
        bucket *b = (bucket *)(c->buckets[i]);
        while (b) {
            for (size_t i = 0; i < b->index; i++) {
                tag *t = (tag *)(b->space + (i * b->units));
                found += update(c, t);
            }
            b = b->next;
        }
    }

    bigtag *b = (bigtag *)(c->big);
    while(b) {
        found += update(c, &(b->tag));
        b = b->next;
    }

    return found;
}

void *collector_thread(void *arg) {
    collector *c = arg;
    collector_cycle( c, update_to_unchecked );

    while (!c->stopped) {
        // iterate roots setting to *_TO_CHECK
        for (size_t i = 0; i < c->root_index; i++) {
            c->roots[i]->state = GC_ACTIVE_TO_CHECK;
        }

        size_t found = 1;
        while (found) {
            while( found ){
                found = collector_cycle( c, update_to_checked );
            }

            // change epoch
            uint8_t e = c->epoch;
            c->epoch = e ? 0 : 1;

            // wait for old epoch to finish (if it hits 1 it is finished)
            while (c->epochs[e] > 1) sleep(0);

            // One last check, there is at least one scenario that can result
            // in to_check's being present.
            // GC               ThreadA         ThreadB
            // ------------+-----------------+-------------
            //                  join_epoch
            //                                  join_epoch
            // cont = uc
            // item = uc
            //                                  get cont
            //                                  cont = tc
            //                  get cont
            //                  cont = tc
            //                  get item
            //                                  delete item from cont
            //                                  leave_epoch
            // check cont
            // cont = cd
            // item is not reachable
            // cycle ends 1+
            // cycle ends 0
            // <------------- We are here --------------->
            //                  item = tc
            //                  leave_epoch
            //
            // The epoch system gives us a syncronization point at which we can
            // be sure operations have completed. Once we complete an epoch and
            // find no TO_CHECK items we know it is safe to clear anything that
            // is still UNCHECKED.
            found = collector_cycle( c, update_to_checked );
        }

        // Anything still GC_UNCHECKED is garbage (in buckets and/or big)
        // This will also reset things to UNCHECKED
        collector_cycle( c, update_to_free );
    }
}
