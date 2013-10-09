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
typedef void  (gc_destructor)( void *arg, void *alloc );

// Create start/pause a collector
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
);

// free_collector will return the destarg in case you
// need to free it as well.
void start_collector( collector *c );
void *free_collector( collector *c );

// Get a pointer to memory at least 'size' bytes large
// The root variant should be used to allocate root objects (never
// destroyed) It cannot be used after the collector is started.
// The root variant also is not multithread-safe,
// only use it in one thread at a time!
void *gc_alloc_root( collector *c, size_t size );
void *gc_alloc     ( collector *c, size_t size );

// Operations that may cause objects references to be removed must be
// wrapped with this.
uint8_t gc_join_epoch ( collector *c );
void    gc_leave_epoch( collector *c, uint8_t e );

/*\ *** WARNING ***
 * The following all ASSUME that the pointers passed to them are pointers
 * provided by gc_alloc. If you pass them other pointers you WILL touch
 * randomish memory.
\*/

// You may use this for anything you want.
// A good use of this might be to identify what kind of data is stored in the
// memory.
// The pad is 2-bytes long.
uint8_t *gc_pad( void *alloc );

// Activate an allocation when you make a reference to it not reachable via a
// 'root' object.
// Deactivate an allocation when you remove a reference that is not reachable
// via a root object.
// Objects are returned from gc_alloc already 'activated', you MUST call
// deactivate on them at some point or they will leak.
void gc_activate  ( void *alloc );
void gc_deactivate( void *alloc );

#endif
