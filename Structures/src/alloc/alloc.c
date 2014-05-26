#include "alloc.h"
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"
#include "../include/gsd_struct_result.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

alloc *alloc_create(size_t item_size, size_t item_count, prm *prm) {
    alloc *out = malloc(sizeof(alloc));
    if (!out) return NULL;

    out->item_size  = item_size;
    out->item_count = item_count;
    out->prm = prm;

    out->group = alloc_create_group(out, NULL);
    if (!out->group) {
        free(out);
        return NULL;
    }

    return out;
}

alloc_group *alloc_create_group(alloc *a, alloc_group *grow) {
    size_t chunks_max;
    if (grow) {
        chunks_max = grow->chunks_max;
        if (chunks_max >= 64) {
            chunks_max += 64;
        }
        else {
            chunks_max = 64;
        }
    }
    else {
        chunks_max = 16;
    }

    size_t size = sizeof(alloc_group) + chunks_max * sizeof(alloc_chunk *);
    alloc_group *out = malloc(size);
    if (!out) return NULL;
    memset(out, 0, size);
    out->chunks_max = chunks_max;

    if (grow) {
        for (size_t i = 0; i < grow->chunks_max; i++) {
            // If this fails it is probably an ordering issue. But it should
            // not happen (of course that means it will).
            assert(grow->chunks[i]);
            out->chunks[i] = grow->chunks[i];
        }
    }
    else {
        out->chunks[0] = alloc_create_chunk(a);
        if (!out->chunks[0]) {
            free(out);
            return NULL;
        }
    }

    return out;
}

alloc_chunk *alloc_create_chunk(alloc *a) {
    size_t size = sizeof(alloc_chunk) + (a->item_size * a->item_count);
    alloc_chunk *out = malloc(size);
    if (!out) return NULL;
    memset(out, 0, size);

    out->map = bitmap_create(a->item_count);
    if (!out->map) {
        free(out);
        return NULL;
    }

    out->refcounts = malloc(sizeof(uint32_t) * a->item_count);
    if (!out->refcounts) {
        bitmap_free(out->map);
        free(out);
        return NULL;
    }
    memset(out->refcounts, 0, sizeof(uint32_t) * a->item_count);

    return out;
}

// ref count =1
result alloc_spawn(alloc *a) {
    // get the group
    // iterate chunks
    // find free in chunk, return it
    // if necessary add a new chunk
    // if necessary grow group
}

// ref count +1
result alloc_get(alloc *a, uint32_t idx) {
    uint32_t chunk      = idx / a->item_count;
    uint32_t chunk_idx  = idx % a->item_count;
    uint64_t chunk_byte = chunk_idx * a->item_size;
    void *ptr = a->group->chunks[chunk]->data + chunk_byte;
}

// rec count -1
void alloc_ret(alloc *a, uint32_t idx) {
}

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);


