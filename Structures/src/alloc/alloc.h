#ifndef ALLOC_ALLOC_H
#define ALLOC_ALLOC_H

#include "../include/gsd_struct_alloc.h"

typedef struct alloc_chunk alloc_chunk;
typedef struct alloc_group alloc_group;

struct alloc_chunk {
    bitmap   *map;
    uint8_t  *refcounts;
    uint8_t   data[];
};

struct alloc_group {
    size_t chunks_max;
    alloc_chunk *chunks[];
};

struct alloc {
    size_t item_size;
    size_t item_count;
    size_t ref_bytes;
    size_t offset;

    prm *prm;

    alloc_group *group;
};

alloc_group *alloc_create_group(alloc *a, alloc_group *grow);
alloc_chunk *alloc_create_chunk(alloc *a);

size_t alloc_ref_delta(alloc *a, size_t chunk, uint64_t idx, int8_t delta);

#endif
