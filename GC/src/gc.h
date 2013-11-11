#ifndef GC_H
#define GC_H

#include <pthread.h>
#include <stdint.h>
#include "include/gsd_gc.h"

#define MAX_BUCKET 5

typedef struct tag       tag;
typedef struct bigtag    bigtag;
typedef struct bucket    bucket;
typedef struct collector collector;
typedef struct epochs    epochs;

struct epochs {
    /* not-bitfield
    int8_t epoch;
    int8_t pulse;
    uint8_t active0;
    uint8_t active1;
    uint16_t counter0;
    uint16_t counter1;
    */

    /* bitfield
    // Using this allows for bigger counters, but may be slower and less
    // portable. Order of the fields is important for alignment and to keep GCC
    // from padding the struct.
    */
    unsigned int counter0 : 24;
    unsigned int epoch    : 1;
    unsigned int active0  : 7;
    unsigned int counter1 : 24;
    unsigned int pulse    : 1;
    unsigned int active1  : 7;
};

struct tag {
    uint32_t drefs;

    uint8_t active[2];

    enum {
        GC_FREE = 0,

        GC_UNCHECKED,

        GC_TO_CHECK,
        GC_ACTIVE_TO_CHECK,

        GC_CHECKED,

        GC_DESTROY,
        GC_DESTROYED,
    } state : 8;

    uint8_t pad;
};

struct bigtag {
    bigtag *next;
    tag     tag;
};

struct bucket {
    uint8_t *space;
    size_t   size;
    size_t   units;

    size_t index;

    bucket *next;
};

struct collector {
    gc_iterable *iterable;

    gc_iterator     *get_iterator;
    gc_iterate_next *next;
    gc_iterate_free *free_iterator;

    gc_iterate  *iterate;

    gc_destructor *destroy;
    void          *destarg;

    int       started;
    int       stopped;
    pthread_t sweep_thread;

    enum {
        GC_CYCLE_INITIAL     = 0,
        GC_CYCLE_PULSE       = 1,
        GC_CYCLE_CHECK       = 2,
        GC_CYCLE_EPOCH       = 3,
        GC_CYCLE_FINAL_CHECK = 4,
        GC_CYCLE_FREE        = 5,
        GC_CYCLE_RETURN      = 6,
    } cycle_state;
    int cycle_epoch;

    size_t  bucket_counts;
    bucket *buckets[MAX_BUCKET];
    tag    *free[MAX_BUCKET];

    bigtag *big;

    tag   **roots;
    size_t  root_size;
    size_t  root_index;

    epochs epochs;
};

// These are atomic swaps. True on success, false on failure.
// for the tag update, oldv is updated to contain the updated value on failure.
int atomic_tag_update( tag *tp, tag *oldv, tag newv );
int atomic_epoch_update( epochs *e, epochs *old, epochs new );

int8_t change_epoch( epochs *es );
void   wait_epoch( epochs *es, int8_t e );

unsigned int update_to_unchecked( collector *c, tag *t );
unsigned int update_to_checked  ( collector *c, tag *t );
unsigned int update_to_destroy  ( collector *c, tag *t );
unsigned int destroy_all        ( collector *c, tag *t );

void *sweep_thread(void *arg);
size_t collector_cycle(collector *c, unsigned int (*update)(collector *c, tag *t) );
void return_buckets( collector *c );

tag *gc_tag( void *alloc );

bucket *create_bucket( int units, size_t count );
void free_bucket( bucket *b );

void collector_destroy( collector *c );

void update_drefs_sub( collector *c, void *alloc );
void update_drefs_add( collector *c, void *alloc );
void update_drefs    ( collector *c, void *alloc, int delta );

#endif
