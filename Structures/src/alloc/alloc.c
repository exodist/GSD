#include "alloc.h"
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"
#include "../include/gsd_struct_result.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"

alloc *alloc_create(size_t item_size, size_t item_count, uint8_t ref_bytes, prm *prm) {
    assert(ref_bytes == 0 || ref_bytes == 1 || ref_bytes == 2 || ref_bytes == 4 || ref_bytes == 8);
    alloc *out = malloc(sizeof(alloc));
    if (!out) return NULL;

    out->item_size  = item_size;
    out->item_count = item_count;
    out->ref_bytes  = ref_bytes;
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

    if (a->ref_bytes) {
        out->refcounts = malloc(a->ref_bytes * a->item_count);
        if (!out->refcounts) {
            bitmap_free(out->map);
            free(out);
            return NULL;
        }
        memset(out->refcounts, 0, a->ref_bytes * a->item_count);
    }

    return out;
}

size_t alloc_ref_delta(alloc *a, uint32_t chunk, uint32_t idx, int8_t delta) {
    uint8_t *ptr = &(a->group->chunks[chunk]->refcounts[idx * a->ref_bytes]);
    switch(a->ref_bytes) {
        case 8: return __atomic_add_fetch((uint64_t *)ptr, delta, __ATOMIC_ACQ_REL);
        case 4: return __atomic_add_fetch((uint32_t *)ptr, delta, __ATOMIC_ACQ_REL);
        case 2: return __atomic_add_fetch((uint16_t *)ptr, delta, __ATOMIC_ACQ_REL);
        case 1: return __atomic_add_fetch((uint8_t  *)ptr, delta, __ATOMIC_ACQ_REL);

        default:
            assert(a->ref_bytes == 0);
        return 0;
    }
}

// ref count =1
int64_t alloc_spawn(alloc *a) {
    uint8_t epoch = prm_join_epoch(a->prm);

    while(1) {
        alloc_group *g = a->group; //TODO: Atomic
        int64_t chunk_idx;
        size_t  chunk;
        for (chunk = 0; chunk < g->chunks_max; chunk++) {
            if (!g->chunks[chunk]) {
                alloc_chunk *c = alloc_create_chunk(a);
                if (!c) {
                    prm_leave_epoch(a->prm, epoch);
                    return -1;
                }
                g->chunks[chunk] = c; //TODO: Atomic Swap from NULL || free new
            }

            chunk_idx = bitmap_fetch(g->chunks[chunk]->map, 0, 0);
            if (chunk_idx >= 0) break;
        }

        //TODO: Grow group
        if (chunk_idx < 0) {
            alloc_group *ng = alloc_create_group(a, g);
            if (!ng) {
                prm_leave_epoch(a->prm, epoch);
                return -1;
            }

            a->group = ng; //TODO: Swap && dispose || free new

            continue;
        }

        assert(g->chunks[chunk]);

        prm_leave_epoch(a->prm, epoch);
 
        return (chunk * a->item_count) + chunk_idx;
    }
}

// ref count +1
void *alloc_get(alloc *a, uint32_t idx) {
    uint32_t chunk      = idx / a->item_count;
    uint32_t chunk_idx  = idx % a->item_count;
    uint64_t chunk_byte = chunk_idx * a->item_size;

    if (a->ref_bytes) assert(alloc_ref_delta(a, chunk, chunk_idx, 1) >= 1);

    uint8_t epoch = prm_join_epoch(a->prm);
    void *out = a->group->chunks[chunk]->data + chunk_byte;
    prm_leave_epoch(a->prm, epoch);
    return out;
}

// rec count -1
void alloc_ret(alloc *a, uint32_t idx) {
    uint32_t chunk      = idx / a->item_count;
    uint32_t chunk_idx  = idx % a->item_count;
    uint8_t epoch = prm_join_epoch(a->prm);

    if((a->ref_bytes)) {
        size_t count = alloc_ref_delta(a, chunk, chunk_idx, 1);
        if (count) return;
    }

    bitmap_set(a->group->chunks[chunk]->map, chunk_idx, 0);
    prm_leave_epoch(a->prm, epoch);
}

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);


