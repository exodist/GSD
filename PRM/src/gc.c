#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "include/gsd_gc.h"
#include "gc.h"

epochs load_c_epochs( collector *c ) {
    epochs out;
    __atomic_load( &(c->epochs), &out, __ATOMIC_CONSUME );
    return out;
}

epochs load_epochs( epochs *es ) {
    epochs out;
    __atomic_load( es, &out, __ATOMIC_CONSUME );
    return out;
}

int atomic_epoch_update( epochs *ep, epochs *old, epochs new ) {
    return __atomic_compare_exchange( ep, old, &new, 0, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME );
}

tag load_tag( tag *t ) {
    tag out;
    __atomic_load( (uint32_t *)t, (uint32_t *)&out, __ATOMIC_CONSUME );
    return out;
}

int atomic_tag_update( tag *tp, tag *old, tag new ) {
    return __atomic_compare_exchange(
        (uint32_t *)tp,
        (uint32_t *)old,
        (uint32_t *)&new,
        0,
        __ATOMIC_ACQ_REL,
        __ATOMIC_CONSUME
    );
}

uint32_t gc_get_pad( void *alloc ) {
    tag *t = gc_tag(alloc);
    return __atomic_load_n(&(t->pad), __ATOMIC_CONSUME);
}

int gc_set_pad( void *alloc, uint32_t *old, uint32_t new ) {
    tag *t = gc_tag(alloc);
    return __atomic_compare_exchange_n( &(t->pad), old, new, 0, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME );
}

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

    for( int i = 0; i < MAX_BUCKET; i++ ) {
        c->buckets[i] = create_bucket( i + 1, bucket_counts );
        if (!c->buckets[i]) {
            for (int j = 0; j < i; j++) free_bucket(c->buckets[j], NULL, NULL);
            free(c);
            return NULL;
        }
    }

    return c;
}

void start_collector( collector *c ) {
    if (__atomic_fetch_add( &(c->started), 1, __ATOMIC_SEQ_CST))
        return;

    // Make sure we have at least one root
    assert( c->root_size && c->root_index && c->roots[0] );

    // create the pthread and start it
    assert( !pthread_create( &(c->sweep_thread),  NULL, sweep_thread,  c ));
    assert( !pthread_create( &(c->bucket_thread), NULL, bucket_thread, c ));
    __atomic_store_n( &(c->started), 1, __ATOMIC_SEQ_CST );
}

void *free_collector( collector *c ) {
    assert(!__atomic_fetch_add( &(c->stopped), 1, __ATOMIC_SEQ_CST));

    // Make sure no epochs are active
    epochs e = load_c_epochs(c);
    while ( e.counter0 > 1 || e.counter1 > 1) {
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 10 };
        select( 0, NULL, NULL, NULL, &timeout );
        e = load_c_epochs(c);
    }

    // wait on pthread
    pthread_join( c->sweep_thread, NULL );
    pthread_join( c->bucket_thread, NULL );

    // free buckets
    for (size_t i = 0; i < MAX_BUCKET; i++) {
        if ( !c->buckets[i] ) {
            continue;
        }
        free_bucket(c->buckets[i], c->destroy, c->destarg);
    }

    if (c->release) {
        release_bucket(c->release);
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
    assert( !__atomic_load_n( &(c->started), __ATOMIC_SEQ_CST ));

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

void gc_activate( collector *c, void *alloc, int8_t e ) {
    tag *t = gc_tag(alloc);

    tag old = load_tag(t);
    while (1) {
        tag new = old;

        epochs es = load_c_epochs(c);
        new.active[e] = e ? es.active1 : es.active0;
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

        if (atomic_tag_update(t, &old, new)) break;
    }
}

int8_t gc_join_epoch( collector *c ) {
    epochs old = load_c_epochs(c);
    while( 1 ) {
        epochs new = old;

        new.pulse = 1;
        switch(new.epoch) {
            case 0:
                new.counter0++;
                if (new.counter0 < old.counter0) return -1;
            break;

            case 1:
                new.counter1++;
                if (new.counter1 < old.counter1) return -1;
            break;
        }

        if(atomic_epoch_update( &(c->epochs), &old, new )) {
            return new.epoch;
        }
    }
}

void gc_leave_epoch( collector *c, int8_t e ) {
    epochs old = load_c_epochs(c);
    while( 1 ) {
        epochs new = old;

        new.pulse = 1;
        switch(e) {
            case 0:
                new.counter0--;
                if (new.counter0 > old.counter0) {
                    fprintf( stderr, "Invalid attempt to leave epoch!\n" );
                    abort();
                }
            break;

            case 1:
                new.counter1--;
                if (new.counter1 > old.counter1) {
                    fprintf( stderr, "Invalid attempt to leave epoch!\n" );
                    abort();
                };
            break;

            default:
                fprintf( stderr, "Attempt to leave epoch '%i'.", e );
                abort();
        }

        if(atomic_epoch_update( &(c->epochs), &old, new )) {
            return;
        }
    }
}

int8_t change_epoch( epochs *es ) {
    epochs old = load_epochs(es);
    while(1) {
        epochs new = old;
        new.epoch = old.epoch ? 0 : 1;
        new.pulse = 0;

        switch(new.epoch) {
            case 0:
                assert( !new.counter0 ); // Make sure new epoch is empty
                new.active0++;
            break;

            case 1:
                assert( !new.counter1 ); // Make sure new epoch is empty
                new.active1++;
            break;
        }

        if (atomic_epoch_update(es, &old, new)) {
            return old.epoch;
        }
    }
}

void wait_epoch( epochs *es, int8_t e ) {
    assert( e == 0 || e == 1 );
    epochs v = load_epochs(es);
    switch(e) {
        case 0:
            while( v.counter0 ) {
                struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
                select( 0, NULL, NULL, NULL, &timeout );
                v = load_epochs(es);
            }
        break;

        case 1:
            while( v.counter1 ) {
                struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
                select( 0, NULL, NULL, NULL, &timeout );
                v = load_epochs(es);
            }
        break;
    }
}

void *gc_alloc( collector *c, size_t size, int8_t e ) {
    uint8_t b = size / 8;

    tag *out = NULL;

    if ( b < MAX_BUCKET ) {
        while (1) {
            __atomic_load( c->free + b, &out, __ATOMIC_CONSUME );
            if (!out) break;

            tag *next = out + 1;

            // Claim the tag, put the next free item in front
            if (__atomic_compare_exchange(
                c->free + b,
                &out,
                &next,
                0,
                __ATOMIC_ACQ_REL,
                __ATOMIC_RELAXED
            )) break;
        }

        if (!out) {
            while(1) {
                bucket *bk = __atomic_load_n(c->buckets + b, __ATOMIC_CONSUME);
                size_t idx = bk->index;

                // Another thread will add a new bucket
                if (idx > bk->size) continue;

                if (!__atomic_compare_exchange_n(
                    &(bk->index),
                    &idx,
                    idx + bk->units,
                    0,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_RELAXED
                )) continue;

                if (idx < bk->size) { // EASY!
                    out = (tag *)(bk->space + idx);
                    break;
                }

                // We drew the short straw, time to add a new bucket
                bucket *nbk = create_bucket( b + 1, c->bucket_counts );
                if (!nbk) {
                    // Reset the index so that another thread can try
                    __atomic_store_n( &(bk->index), idx, __ATOMIC_RELEASE );
                    return NULL; // Out of memory!
                }

                __atomic_store( &(nbk->next), &bk, __ATOMIC_RELEASE );
                __atomic_store_n( &(nbk->index), bk->units, __ATOMIC_RELEASE );
                out = (tag *)(nbk->space + 0);
                assert(__atomic_compare_exchange_n(
                    c->buckets + b,
                    &bk,
                    nbk,
                    0,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_RELAXED
                ));
                break;
            }
        }
    }
    // Have to do a bigtag
    else {
        b = -1;
        bigtag *o = malloc( sizeof(bigtag) + size );
        if (!o) return NULL;
        __atomic_load(&(c->big), &(o->next), __ATOMIC_CONSUME);
        while(1) {
            int success = __atomic_compare_exchange_n(
                &(c->big),
                o->next,
                o,
                0,
                __ATOMIC_ACQ_REL,
                __ATOMIC_CONSUME
            );
            if (success) {
                out = &(o->tag);
                break;
            }
        }
    }

    epochs es = load_c_epochs(c);
    tag old = load_tag( out );
    tag new = old;

    assert( new.state == GC_FREE );
    new.state     = GC_CHECKED;
    new.active[e] = e ? es.active1 : es.active0;
    new.bucket    = b;

    assert(atomic_tag_update(out, &old, new));
    __atomic_store_n( &(out->pad), 0, __ATOMIC_RELEASE );

    out++;
    memset(out, 0, size);

    return out;
}

unsigned int update_to_unchecked( collector *c, tag *t ) {
    if (t->state == GC_FREE)
        return 0;

    tag old = load_tag( t );
    while (1) {
        tag new = old;

        epochs es = load_c_epochs(c);
        new.state = old.active[0] == es.active0 || old.active[1] == es.active1
            ? GC_UNCHECKED
            : GC_ACTIVE_TO_CHECK;

        if (atomic_tag_update(t, &old, new)) break;
    }

    return 1;
}

unsigned int update_to_checked( collector *c, tag *t ) {
    tag old = load_tag( t );
    while (1) {
        tag new = old;

        switch( old.state ) {
            case GC_FREE: return 0;

            case GC_TO_CHECK:
            case GC_ACTIVE_TO_CHECK:
                new.state = GC_CHECKED;
            break;

            default: return 0;
        }

        if (atomic_tag_update(t, &old, new)) break;
    }

    void *iterator = NULL;
    switch(c->iterable( t + 1 )) {
        case GC_NONE: break;

        case GC_ITERATOR:
            iterator = c->get_iterator( t + 1 );
            assert(iterator);
            tag *it = c->next( iterator );
            while(it) {
                it--; // next returns the alloc, we -- to get the actual tag
                tag old = load_tag( it );

                // Update it to checked
                while(old.state != GC_TO_CHECK && old.state != GC_ACTIVE_TO_CHECK) {
                    assert( old.state != GC_FREE );
                    tag new = old;

                    epochs es = load_c_epochs(c);
                    new.state = old.active[0] == es.active0 || old.active[1] == es.active1
                        ? GC_ACTIVE_TO_CHECK
                        : GC_TO_CHECK;

                    if (atomic_tag_update(t, &old, new)) break;
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
    tag old = load_tag( t );
    while (1) {
        tag new = old;

        switch( old.state ) {
            case GC_FREE: return 0;

            case GC_UNCHECKED:
                new.state = GC_FREE;
            break;

            default: return update_to_unchecked( c, t );
        }

        if (atomic_tag_update(t, &old, new)) break;
    }

    if (c->destroy) {
        c->destroy( t + 1, c->destarg );
    }

    return 1;
}

size_t collector_cycle(collector *c, unsigned int (*update)(collector *c, tag *t) ) {
    size_t found = 0;

    for (size_t i = 0; i < c->root_index; i++) {
        found += update(c, c->roots[i]);
    }

    for(int i = 0; i < MAX_BUCKET; i++) {
        bucket *b = NULL;
        __atomic_load( c->buckets + i, &b, __ATOMIC_ACQUIRE );
        while (b) {
            for (size_t i = 0; i < c->bucket_counts; i++) {
                tag *ti = (tag *)(b->space + (i * b->units));
                found += update(c, ti);
            }
            __atomic_load( &(b->next), &b, __ATOMIC_ACQUIRE );
        }
    }

    bigtag *b = (bigtag *)(c->big);
    while(b) {
        found += update(c, &(b->tag));
        b = b->next;
    }

    return found;
}

void *sweep_thread(void *arg) {
    collector *c = arg;

    collector_cycle( c, update_to_unchecked );

    while (!__atomic_load_n(&(c->stopped), __ATOMIC_CONSUME)) {
        epochs es = load_c_epochs(c);
        if(!(es.pulse || __atomic_load_n(&(c->stopped), __ATOMIC_CONSUME))) {
            struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
            select( 0, NULL, NULL, NULL, &timeout );
        }

        // iterate roots setting to *_TO_CHECK
        for (size_t i = 0; i < c->root_index; i++) {
            tag old = load_tag( c->roots[i] );
            while(1) {
                tag new = old;
                new.state = GC_ACTIVE_TO_CHECK;
                if(atomic_tag_update( c->roots[i], &old, new )) break;
            }
        }

        size_t found = 1;
        while (found) {
            while( found ){
                found = collector_cycle( c, update_to_checked );
            }

            int8_t e = change_epoch( &(c->epochs) );
            wait_epoch( &(c->epochs), e );

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
        }

        // Anything still GC_UNCHECKED is garbage (in buckets and/or big)
        // This will also reset things to UNCHECKED
        collector_cycle( c, update_to_free );

        bigtag *b = NULL;
        __atomic_load( &(c->big), &b, __ATOMIC_CONSUME );
        if (b) {
            while (b->next) {
                bigtag *n = b->next;

                // Next is not free, move on
                if (n->tag.state != GC_FREE) {
                    b = n;
                    continue;
                }

                // No need ot be atomic, apart from c->big itself none of these
                // links are read outside this thread.
                b->next = n->next;
                // The destructor will have already run in the update_to_free
                // cycle
                free(n);
            }
        }

        bucket *release = NULL;
        __atomic_load( &(c->release), &release, __ATOMIC_ACQUIRE );
        if (release) {
            while(1) {
                int ok = __atomic_compare_exchange_n(
                    &(c->release),
                    &release,
                    NULL,
                    0,
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_ACQUIRE
                );
                if (ok) break;
            }
        }
        if (release) release_bucket(release);
    }

    return NULL;
}

bucket *create_bucket( int units, size_t count ) {
    assert(units);
    bucket *out = malloc( sizeof(bucket) );
    if (!out) return NULL;
    memset(out, 0, sizeof(bucket));

    units *= 8; // 64-bytes per unit.
    units += 8; // Add the tag

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

void release_bucket( bucket *b ) {
    while (b) {
        bucket *kill = b;
        b = b->release;
        if( kill->space ) free(kill->space);
        free(kill);
    }
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

void *bucket_thread(void *arg) {
    collector *c = arg;

    while (!__atomic_load_n(&(c->stopped), __ATOMIC_CONSUME)) {
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 200 };
        select( 0, NULL, NULL, NULL, &timeout );

        for ( int i = 0; i < MAX_BUCKET; i++ ) {
            // Nullify the free list
            __atomic_store_n( c->free + i, NULL, __ATOMIC_RELEASE );

            // Iterate buckets
            bucket *l = NULL;
            __atomic_load( c->buckets + i, &l, __ATOMIC_CONSUME );
            if (!l) continue;

            bucket *b = l->next;

            while (b) {
                tag *free = NULL;
                size_t nonfree = 0;

                size_t max = b->index / b->units;
                if (max != 0) max -= 1;

                for (size_t i = 0; i < c->bucket_counts; i++) {
                    tag *ti = (tag *)(b->space + (i * b->units));
                    tag  t = load_tag( ti );
                    if ( t.state == GC_FREE ) {
                        tag **next = (void *)(ti + 1);
                        *next = free;
                        free = ti;
                    }
                    else {
                        nonfree++;
                    }
                }
                if (nonfree) {
                    tag **slot = (c->free + i);

                    tag *first = (tag *)(b->space);
                    tag **next = (void *)(first + 1);

                    __atomic_load( slot, next, __ATOMIC_ACQUIRE );
                    while(1) {
                        int ok = __atomic_compare_exchange_n(
                            slot,
                            next,
                            free,
                            0,
                            __ATOMIC_ACQ_REL,
                            __ATOMIC_ACQUIRE
                        );
                        if (ok) break;
                    }
                    l = b;
                }
                else {
                    // Remove b from the list
                    __atomic_store( &(l->next), &(b->next), __ATOMIC_RELEASE );

                    __atomic_load( &(c->release), &(b->release), __ATOMIC_ACQUIRE );
                    while(1) {
                        int ok = __atomic_compare_exchange_n(
                            &(c->release),
                            &(b->release),
                            b,
                            0,
                            __ATOMIC_ACQ_REL,
                            __ATOMIC_ACQUIRE
                        );
                        if (ok) break;
                    }
                }

                b = l->next;
            }
        }
    }

    return NULL;
}
