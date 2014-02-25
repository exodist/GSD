#ifndef PRM_H
#define PRM_H

#include <stdlib.h>

#include "../include/gsd_struct_prm.h"

typedef struct epoch     epoch;
typedef struct prm       prm;
typedef struct trash_bin trash_bin;
typedef struct trash_bag trash_bag;

struct trash_bin {
    prm       *prm;
    trash_bag *trash_bags;
    int16_t    dep;
};

struct prm {
    size_t thread_at;
    size_t detached_threads;

    epoch   *epochs;
    size_t   size;
    uint8_t  count;

    uint8_t current;
};

struct epoch {
    /*\
     * active count of 0 means free
     * active count of 1 means needs to be cleared (not joinable)
     * active count of >1 means joinable
    \*/
    trash_bag *trash_bags;
    size_t active;

    int16_t dep;
};

struct trash_bag {
    uint64_t destructor_map;
    void   **garbage;
    size_t idx;

    trash_bag *next;
};

uint8_t advance_epoch( prm *p, uint8_t e );

trash_bag *new_trash_bag(prm *p, uint8_t epoch, uint8_t slots);

void *garbage_truck( void *args );
void free_garbage( prm *s, trash_bag *b );

#endif

