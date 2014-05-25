#include "alloc.h"
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"
#include "stdlib.h"
#include "string.h"

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
    memset(out, size);
    out->chunks_max = chunks_max;

    if (grow) {
        for (size_t i = 0; i < grow->chunks_max; i++) {
            // TODO: ATOMIC
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

}

// ref count =1
result alloc_spawn(alloc *a) {
}

// ref count +1
result alloc_get(alloc *a, uint32_t idx) {
}

// rec count -1
void alloc_ret(alloc *a, uint32_t idx) {
}

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);


