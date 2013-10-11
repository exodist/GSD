#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
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
    void          *destarg,

    size_t bucket_counts
) {
    // Code later relies on this assertion, but we put it here to keep it from
    // running more than once.
    assert(sizeof(tag) == 8);

    collector *c = malloc(sizeof(collector));
    if (!c) return NULL;

    memset(c, 0, sizeof(collector));

    c->iterable      = iterable;
    c->get_iterator  = get_iterator;
    c->next          = next;
    c->iterate       = iterate;
    c->callback      = callback;
    c->destroy       = destroy;
    c->destarg       = destarg;
    c->bucket_counts = bucket_counts;

    c->active[0] = 1;
    c->active[1] = 1;

    for( int i = 0; i < MAX_BUCKET; i++ ) {
        c->buckets[i] = create_bucket( i + 1, bucket_counts );
        if (!c->buckets[i]) {
            for (int j = 0; j < i; j++) free_bucket((bucket *)(c->buckets[j]), NULL, NULL);
            free(c);
            return NULL;
        }
    }

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
    while ( c->epochs[0] > 1 || c->epochs[1] > 1) {
        sleep(0);
    }

    // wait on pthread
    pthread_join( c->thread, NULL );

    // free buckets
    for (size_t i = 0; i < MAX_BUCKET; i++) {
        if ( !c->buckets[i] ) {
            continue;
        }
        free_bucket((bucket *)(c->buckets[i]), c->destroy, c->destarg);
    }

    // free bigs
    bigtag *bt = (bigtag *)(c->big);
    while (bt) {
        bigtag *kill = bt;
        bt = bt->next;
        if (c->destroy) c->destroy( kill, c->destarg );
        free(kill);
    }

    // free roots
    for( size_t i = 0; i < c->root_index; i++ ) {
        if (c->destroy) {
            tag *r = c->roots[i];
            c->destroy( r + 1, c->destarg );
        }
        free( c->roots[i] );
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
    alloc->bucket = -1;
    alloc->state  = GC_CHECKED;
    c->roots[idx] = alloc;

    // Return data section
    return alloc + 1;
}

tag *gc_tag( void *alloc ) {
    tag *t = alloc;
    return t - 1;
}

uint32_t gc_get_pad( void *alloc ) {
    return (uint32_t)(gc_tag(alloc)->pad);
}

void gc_set_pad( void *alloc, uint32_t val ) {
    tag *t = gc_tag(alloc);
    while (1) {
        tag old = *t;
        tag new = old;
        new.pad = val;
        if (atomic_tag_update( t, old, new )) return;
    }
}

int atomic_tag_update( tag *tp, tag oldv, tag newv ) {
    union { tag *t; uint64_t *i; } t;
    union { tag  t; uint64_t  i; } old;
    union { tag  t; uint64_t  i; } new;

    t.t = tp;
    old.t = oldv;
    new.t = newv;
    if (old.i == new.i) return 1;

    return __sync_bool_compare_and_swap( t.i, old.i, new.i );
}

void gc_activate( collector *c, void *alloc, uint8_t e ) {
    tag *t = gc_tag(alloc);

    while (1) {
        tag old = *t;
        tag new = old;

        new.active[e] = c->active[e];
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

void *gc_alloc( collector *c, size_t size, uint8_t e ) {
    uint8_t b = size / 8;

    tag *out = NULL;

    if ( b < MAX_BUCKET ) {
        while (1) {
            out = (tag *)(c->free[b]);
            if (!out) break;

            tag *n = out + 1;

            // Claim the tag, put the next free item in front
            if (__sync_bool_compare_and_swap(c->free + b, out, n))
                break;
        }

        while(1) {
            bucket *bk = (bucket *)(c->buckets[b]);
            size_t idx = bk->index;

            // Another thread will add a new bucket
            if (idx > size) continue;

            if (!__sync_bool_compare_and_swap( &(bk->index), idx, idx + bk->units ))
                continue;

            if (idx < size) { // EASY!
                out = (tag *)(bk->space + idx);
                break;
            }

            // We drew the short straw, time to add a new bucket
            bucket *nbk = create_bucket( b + 1, c->bucket_counts );
            if (!nbk) {
                // Reset the index so that another thread can try
                assert(__sync_bool_compare_and_swap( &(bk->index), idx + bk->units, idx));
                return NULL; // Out of memory!
            }

            out = (tag *)(nbk->space + 0);
            assert(__sync_bool_compare_and_swap( c->buckets + b, bk, nbk));
            break;
        }
    }
    // Have to do a bigtag
    else {
        b = -1;
        bigtag *o = malloc( sizeof(bigtag) + size );
        if (!o) return NULL;
        while(1) {
            o->next = (bigtag *)(c->big);
            if (__sync_bool_compare_and_swap(&(c->big), o->next, o)) {
                out = &(o->tag);
                break;
            }
        }
    }

    while( 1 ) {
        tag old = *out;
        tag new = old;
        new.state  = GC_CHECKED;
        new.active[e] = c->active[e];
        new.bucket = b;
        new.pad    = 0;
        if (atomic_tag_update(out, old, new)) break;
    }
    out++;
    memset(out, 0, size);

    return out;
}

unsigned int update_to_unchecked( collector *c, tag *t ) {
    if (t->state == GC_FREE)
        return 0;

    while (1) {
        tag old = *t;
        tag new = old;

        new.state = old.active[0] == c->active[0] || old.active[1] == c->active[1]
            ? GC_UNCHECKED
            : GC_ACTIVE_TO_CHECK;

        // Atomic swap( t, old, new );
        if (atomic_tag_update(t, old, new)) break;
    }

    return 1;
}

unsigned int update_to_checked( collector *c, tag *t ) {
    while (1) {
        tag old = *t;
        tag new = old;

        switch( old.state ) {
            case GC_FREE: return 0;

            case GC_TO_CHECK:
            case GC_ACTIVE_TO_CHECK:
                new.state = GC_CHECKED;
            break;

            default: return 0;
        }

        // Atomic swap( t, old, new );
        if (atomic_tag_update(t, old, new)) break;
    }

    void *iterator = NULL;
    switch(c->iterable( t + 1 )) {
        case GC_NONE: break;

        case GC_ITERATOR:
            iterator = c->get_iterator( t + 1 );
            assert(iterator);
            tag *it = c->next( iterator );
            while(it) {
                it++; // next returns the alloc, we -- to get the actual tag

                // Update it to checked
                while(it->state != GC_TO_CHECK && it->state != GC_ACTIVE_TO_CHECK) {
                    assert( it->state != GC_FREE );
                    tag old = *it;
                    tag new = old;

                    new.state = old.active[0] == c->active[0] || old.active[1] == c->active[1]
                        ? GC_ACTIVE_TO_CHECK
                        : GC_TO_CHECK;

                    // Atomic swap( t, old, new );
                    if (atomic_tag_update(t, old, new)) break;
                }

                it = c->next( iterator );
            }
        break;

        case GC_CALLBACK:
            c->iterate( t + 1, c->callback );
        break;
    }

    return 1;
}

unsigned int update_to_free( collector *c, tag *t ) {
    while (1) {
        tag old = *t;
        tag new = old;

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

    if (c->destroy) {
        c->destroy( t + 1, c->destarg );
    }

    // This is a bigtag, it will be cleaned up later
    if (t->bucket == -1) return 1;

    // Push the tag to the front of the free list.
    tag **slot = (tag **)(c->free + t->bucket);
    while (1) {
        // All tags are the 64-bit tag, followed by (bucket+1)*64 bits
        tag *next = t + 1;
        next = *slot;
        if(__sync_bool_compare_and_swap( slot, next, t )) break;
    }

    return 1;
}

size_t collector_cycle(collector *c, unsigned int (*update)(collector *c, tag *t) ) {
    size_t found = 0;

    for (size_t i = 0; i < c->root_index; i++) {
        found += update(c, c->roots[i]);
    }

    for(int i = 0; i < MAX_BUCKET; i++) {
        bucket *b = (bucket *)(c->buckets[i]);
        while (b) {
            for (size_t i = 0; i < b->index / b->units; i++) {
                tag *ti = (tag *)(b->space + (i * b->units));
                found += update(c, ti);
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
            while (c->epochs[e] > 1) {
                sleep(0);
            }
            // Change the number that sets a tag as active for this epoch.
            // But never let it be 0.
            // This will cycle around and possibly make old items from a
            // previosu cycle seem active, this false-positive is ok, it just
            // means a delay in collection till the next epoch change.
            c->active[e] = c->active[e] == 256 ? 1 : c->active[e] + 1;

            // Close out epoch if necessary
            if (c->epochs[e]) {
                assert( __sync_bool_compare_and_swap( c->epochs + e, 1, 0 ));
            }

            /*\ Explanation for final cycle:
             * One last check, there is at least one scenario that can result
             * in to_check's being present.  The 3 operations below surrounded
             * with exclamation points are key.
             * 
             * GC          |    ThreadA      |  ThreadB
             * ------------+-----------------+-------------
             *                  join_epoch
             *                                  join_epoch
             * cont = uc
             * item = uc
             *                                  get cont
             *                                  cont = tc
             *                  get cont
             *                  cont = tc
             *                  !get item!
             *                                  !delete item from cont!
             * !check cont!
             * cont = cd
             * item is not reachable
             * cycle ends 1+
             * cycle ends 0
             * <------------- We are here --------------->
             *                  item = tc
             *                  leave_epoch
             * 
             * The epoch system gives us a syncronization point at which we can
             * be sure operations have completed. Once we complete an epoch and
             * find no TO_CHECK items we know it is safe to clear anything that
             * is still UNCHECKED.
            \*/

            found = collector_cycle( c, update_to_checked );
            sleep(1);
        }

        // Anything still GC_UNCHECKED is garbage (in buckets and/or big)
        // This will also reset things to UNCHECKED
        collector_cycle( c, update_to_free );

        bigtag *b = (bigtag *)(c->big);
        if (b) {
            while (b->next) {
                bigtag *n = b->next;

                // Next is not free, move on
                if (n->tag.state != GC_FREE) {
                    b = n;
                    continue;
                }

                b->next = n->next;
                // The destructor will have already run in the update_to_free
                // cycle
                free(n);
            }
        }
    }

    return NULL;
}

bucket *create_bucket( int units, size_t count ) {
    assert(units);
    bucket *out = malloc( sizeof(bucket) );
    if (!out) return NULL;
    memset(out, 0, sizeof(bucket));

    units *= 8; // 64-bytes per unit.

    out->space = malloc( count * units );
    if (!out->space) {
        free(out);
        return NULL;
    }
    memset( out->space, 0, count * units );

    out->units = units;
    out->size = count * units;

    return out;
}

void free_bucket( bucket *b, gc_destructor *destroy, void *destarg ) {
    while (b) {
        bucket *kill = b;
        b = b->next;
        if (destroy) {
            for (size_t i = 0; i < kill->index; i += kill->units) {
                tag *t = (tag *)(kill->space + i);
                if (t->state == GC_FREE) continue;
                if (destroy) {
                    destroy( t + 1, destarg );
                }
            }
        }
        if( kill->space ) free(kill->space);
        free(kill);
    }
}
