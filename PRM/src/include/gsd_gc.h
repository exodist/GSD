#ifndef GSD_GC_H
#define GSD_GC_H

#include <stdlib.h>
#include <stdint.h>

typedef struct collector collector;

// Various callback functions that are needed
typedef void (gc_callback)( collector *c, void *alloc );
typedef enum { GC_NONE, GC_ITERATOR, GC_CALLBACK } iteration_type;
typedef iteration_type (gc_iterable)( void *alloc );
typedef void *(gc_iterator)( void *alloc );
typedef void  (gc_iterate_free)( void *iterator );
typedef void *(gc_iterate_next)( void *iterator );
typedef void  (gc_iterate)( collector *c, void *alloc, gc_callback *callback );

// The destructor should return 1 if it is fine to truly free the object. If
// the destructor did something that will hold on to the object (return 0) then
// you must later call destructor_free(), or destructor_restore() on the object
// to either allow it to be freed, or put it back into circulation.
typedef int (gc_destructor)( void *alloc, void *arg );

// Create start/pause a collector
collector *build_collector(
    // Check if an item is iterable, and how
    gc_iterable *iterable,

    // Iterate items via an iterator object
    gc_iterator     *get_iterator,
    gc_iterate_next *next,
    gc_iterate_free *free_iterator,

    // Objects that need to iterate for you with a callback
    gc_iterate  *iterate,

    gc_destructor *destroy,
    void          *destarg,

    // How many objects to allocate per bucket (higher is better up to a point)
    size_t bucket_counts
);

// Start the collector with a thread that constantly collects garbage
void start_collector_thread( collector *c );
void stop_collector_thread ( collector *c );

// Start the collector, but do not start a collection thread, instead you will
// have to call gc_cycle yourself.
void start_collector( collector *c );
void stop_collector( collector *c );
int gc_cycle( collector *c );
/*\ RETURN CODES:
 * -2 Nothing was done (do something else and try again after a couple
                        milliseconds)

 * -1 Waiting for state (do something else and try again after a couple
                         milliseconds)

 *  0 Cycle advanced but not ready to collect, call again any time

 *  1 ready to free memory, call again to do it

 *  2 ready to return larger blocks to the system, call again to do it

If the code is <0 then you should have the thread do something else, if there
is nothing else you should sleep for a bit to avoid cpu churn.

A code of 0 means stuff was done, and more can be done. You can do something
else, or you can call again.

A code of 1 means it is ready to free memory, you probably want to go ahead and
call it again immedietly, but if there is something very pressing it is ok to
do it first.
\*/

// destroy_collector() will call the destructor for every active object.
// free_collector will return the destarg in case you
// need to free it as well.
void  destroy_collector( collector *c );
void *free_collector( collector *c );

// Get a pointer to memory at least 'size' bytes large
// The root variant should be used to allocate root objects (never
// destroyed) It cannot be used after the collector is started.
// The root variant also is not multithread-safe,
// only use it in one thread at a time!
void *gc_alloc_root( collector *c, size_t size               );
void *gc_alloc     ( collector *c, size_t size, int8_t epoch );

// Operations that may cause objects references to be removed must be
// wrapped with this.
int8_t gc_join_epoch ( collector *c );
void   gc_leave_epoch( collector *c, int8_t e );

/*\ *** WARNING ***
 * The following all ASSUME that the pointers passed to them are pointers
 * provided by gc_alloc. If you pass them other pointers you WILL touch
 * randomish memory.
\*/

// You may use this for anything you want.
// A good use of this might be to identify what kind of data is stored in the
// memory.
// The pad is 1-byte long.
uint8_t gc_get_pad( void *alloc                            );
int     gc_set_pad( void *alloc, uint8_t *old, uint8_t new );

// Activate an allocation when you make a reference to it not reachable via a
// 'root' object.
// Objects are returned from gc_alloc already 'activated', you MUST call
void gc_activate( collector *c, void *alloc, int8_t epoch );

void destructor_free( void *alloc );
void destructor_restore( collector *c, void *alloc );

#endif
