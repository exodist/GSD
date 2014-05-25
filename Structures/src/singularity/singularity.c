#include "singularity.h"
#include "string.h"
#include "assert.h"

singularity *singularity_create(collector *c, pool *threads, pool *prms, alloc *pointers, int8_t hasherc, ...) {
    singularity *out = malloc(sizeof(singularity));
    if (!out) return NULL;
    memset(out, 0, sizeof(singularity));

    out->collector = c;
    out->threads   = threads;
    out->prms      = prms;
    out->pointers  = pointers;

    va_list hs;
    va_start(hs, hasherc);
    for(int i = 0; i < hasherc; i++) {
        hasher *h = va_arg(hs, hasher *);
        assert(singularity_hasher_push(out, h) >= 0);
    }
    va_end(hs);

    return out;
}

collector *singularity_collector(singularity *s) {
    return s->collector;
}

pool *singularity_threads(singularity *s) {
    return s->threads;
}

pool *singularity_prms(singularity *s) {
    return s->prms;
}

alloc *singularity_pointers(singularity *s) {
    return s->pointers;
}

int8_t singularity_hasherc(singularity *s) {
    return (int8_t)__atomic_load_n(&(s->hasherc), __ATOMIC_CONSUME);
}

hasher *singularity_hasher(singularity *s, int8_t id) {
    assert(id >= 0);
    assert(id < s->hasherc);
    assert(id < 100);
    hasher *out = NULL;
    while(!out) {
        __atomic_load(s->hashers + id, &out, __ATOMIC_CONSUME);
    }
    return out;
}

int8_t singularity_hasher_push(singularity *s, hasher *h) {
    // First do a load to avoid bumping the counter high enough to overflow
    int64_t id = __atomic_load_n(&(s->hasherc), __ATOMIC_CONSUME);
    if (id >= 100) return -1;

    // Update the value and get our id.
    id = __atomic_fetch_add(&(s->hasherc), 1, __ATOMIC_ACQ_REL);
    if (id >= 100) return -1;

    __atomic_store(s->hashers + id, &h, __ATOMIC_RELEASE);
    return (int8_t)id;
}
