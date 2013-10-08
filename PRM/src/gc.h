#ifndef GC_H
#define GC_H

#include <pthread.h>
#include <stdint.h>
#include "include/gsd_gc.h"
#include "include/gsd_prm.h"

typedef struct tag       tag;
typedef struct bigtag    bigtag;
typedef struct bucket    bucket;
typedef struct collector collector;

struct tag {
    uint32_t active : 32;

    enum {
        GC_FREE,

        GC_UNCHECKED,

        GC_TO_CHECK,
        GC_ACTIVE_TO_CHECK,

        GC_CHECKED,
    } state : 8;

    uint8_t pad[3];
};

struct bigtag {
    tag   tag;
    void *next;
};

struct bucket {
    uint8_t *space;
    size_t   size;
    size_t   index;
    size_t   units;

    bucket *next;
};

struct collector {
    gc_iterable *iterable;

    gc_iterator     *get_iterator;
    gc_iterate_next *next;

    gc_iterate  *iterate;
    gc_callback *callback;

    int       started;
    pthread_t thread;

    bucket *buckets[4];
    void   *free[4];

    bigtag *big;

    tag   *roots;
    size_t root_size;
    size_t root_index;

    size_t  epochs[2];
    uint8_t epoch;
};

void *collector_thread(void *arg);

tag *gc_tag( void *alloc );

#endif
