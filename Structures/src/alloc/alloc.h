#ifndef ALLOC_ALLOC_H
#define ALLOC_ALLOC_H

#include "../include/gsd_struct_alloc.h"

typedef struct alloc_chunk alloc_chunk;
typedef struct alloc_group alloc_group;

struct alloc_chunk {
    bitmap   *map;
    uint32_t *refcounts;
    uint8_t  *data;
};

struct alloc_group {
    size_t chunks_max;
    size_t chunk_idx;
    alloc_chunk *chunks[];
};

struct alloc {
    size_t item_size;
    size_t item_count;

    prm *prm;

    alloc_group *group;
};

alloc_group *alloc_create_group(alloc *a, alloc_group *grow);
alloc_chunk *alloc_create_chunk(alloc *a);

#endif