#ifndef GC_H
#define GC_H

#include <pthread.h>
#include <stdint.h>
#include "include/gsd_gc.h"
#include "include/gsd_prm.h"

#define MAX_BUCKET 5

typedef struct tag       tag;
typedef struct bigtag    bigtag;
typedef struct bucket    bucket;
typedef struct collector collector;

struct tag {
    volatile uint32_t active;

    volatile enum {
        GC_FREE,

        GC_UNCHECKED,

        GC_TO_CHECK,
        GC_ACTIVE_TO_CHECK,

        GC_CHECKED,
    } state : 8;

    int8_t bucket;

    uint8_t pad[2];
};

struct bigtag {
    bigtag *next;
    tag     tag;
};

struct bucket {
    uint8_t *space;
    size_t   size;
    size_t   units;

    volatile size_t index;

    bucket *next;
};

struct collector {
    gc_iterable *iterable;

    gc_iterator     *get_iterator;
    gc_iterate_next *next;

    gc_iterate  *iterate;
    gc_callback *callback;

    gc_destructor *destroy;
    void          *destarg;

    int          started;
    volatile int stopped;
    pthread_t    thread;

    size_t           bucket_counts;
    volatile bucket *buckets[MAX_BUCKET];
    volatile tag    *free[MAX_BUCKET];

    volatile bigtag *big;

    tag   **roots;
    size_t  root_size;
    size_t  root_index;

    volatile size_t  epochs[2];
    volatile uint8_t epoch;
};

int atomic_tag_update( tag *t, tag old, tag new );

unsigned int update_to_unchecked( collector *c, tag *t );
unsigned int update_to_checked  ( collector *c, tag *t );
unsigned int update_to_free     ( collector *c, tag *t );

void *collector_thread(void *arg);
size_t collector_cycle(collector *c, unsigned int (*update)(collector *c, tag *t) );

tag *gc_tag( void *alloc );

bucket *create_bucket( int units, size_t count );
void free_bucket( bucket *b, gc_destructor *destroy, void *destroyarg );

#endif
