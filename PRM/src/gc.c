#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
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
    gc_callback *callback
) {
    // Allocate collector
    // set to 0
    // assign callbacks and stuff
    // return
}

void start_collector( collector *c ) {
    // return if thread is started

    // Make sure we have at least one root
    // create the pthread and start it
}

void *collector_thread(void *arg) {
    // set everything
    // if active set GC_ACTIVE_TO_CHECK
    // else      set GC_UNCHECKED

    // iterate roots setting to GC_TO_CHECK

    // seen = 1;
    // while ( seen ) {
    //      epoch = current;
    //      change epoch;
    //      seen  = 0;
    //      found = 1;
    //      while (found || epoch is active) {
    //          found = 0;
    //          iterate all objects (buckets and big) {
    //              if *_TO_CHECK {
    //                  seen++;
    //                  found++;
    //                  mark = *_CHECKED
    //                  iterate children {
    //                      mark = *_TO_CHECK (unless checked)
    //                  }
    //              }
    //          }
    //          if (!found) {
    //              pthread condition wait?
    //          }
    //      }
    //  }

    // Anything still GC_UNCHECKED is garbage (in buckets and/or big)
}

void *gc_alloc_root( collector *c, size_t size ) {
    // assert that thread is not started

    // use malloc to build these (tag + size)
    // Add allocation to roots
    // set active = 2 (never deactivate)

    // return pointer
}

void *gc_alloc ( collector *c, size_t size ) {
    // if buckets[size - 1] use bucket
    // else malloc and push onto 'big'
}

tag *gc_tag( void *alloc ) {
    tag *t = alloc;
    return t - 1;
}

uint8_t *gc_pad( void *alloc ) {
    return (uint8_t *)(gc_tag(alloc)->pad);
}

void gc_activate ( void *alloc ) {
    tag *t = gc_tag(alloc);

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
}

void gc_deactivate( void *alloc ) {
    tag *t = gc_tag(alloc);

    // Atomic decrement of 
}

void gc_join_epoch ( collector *c ) {
    // ++ current epoch (atomic) (0, 1, 2+ like prm)
}

void gc_leave_epoch( collector *c ) {
    // -- current epoch (atomic)
    // fire off condition when last to leave epoch?
}
