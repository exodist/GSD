#ifndef GSD_GC_H
#define GSD_GC_H

#include <stdlib.h>

typedef struct collector collector;

// Various callback functions that are needed
typedef void (gc_callback)( void *alloc );
typedef enum { GC_NONE, GC_ITERATOR, GC_CALLBACK } (gc_iterable)( void *alloc );
typedef void *(gc_iterator)( void *alloc );
typedef void *(gc_iterate_next)( void *iterator );
typedef void  (gc_iterate)( void *alloc, gc_callback *callback );

// Create start/pause a collector
collector *build_collector(
    // Check if an item is iterable, and how
    gc_iterable *iterable,

    // Iterate items via an iterator object
    gc_iterator     *get_iterator,
    gc_iterate_next *next,

    // Objects that need to iterate for you with a callback
    gc_iterate  *iterate,
    gc_callback *callback
);

void start_collector( collector *c );
void pause_collector( collector *c );

// Add/remove root objects
size_t collector_add_root( collector *c );
void   collector_rem_root( collector *c, size_t idx );

// Get a pointer to memory at least 'size' bytes large
void *gc_alloc( collector *c, size_t size );

// This will actually return the memory to the collector for re-use, do not use
// this if there is a chance other references to the memory exist in this or
// other threads. In a concurrent environment you might want to look at pairing
// the collector with PRM (Parallel Resource Manager). 
void gc_free( collector *c, void *alloc );

/*\ *** WARNING ***
 * The following all ASSUME that the pointers passed to them are pointers
 * provided by gc_alloc. If you pass them other pointers you WILL touch
 * randomish memory.
\*/

// Get a pointer to the 20 bits of padding between the GC tag and the pointer.
// You may use this for anything you want.
// A good use of this might be to identify what kind of data is stored in the
// memory.
void *gc_pad( void *alloc );

// Activate an allocation when you make a reference to it not reachable via a
// 'root' object.
// Deactivate an allocation when you remove a reference that is not reachable
// via a root object.
// Objects are returned from gc_alloc already 'activated', you MUST call
// deactivate on them at some point or they will leak.
void gc_activate  ( void *alloc );
void gc_deactivate( void *alloc );

#endif
