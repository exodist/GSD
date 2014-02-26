#include "pool.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"

pool *pool_create(size_t count, void *(*spawn)(void *arg), void (*free) (void *item), void *spawn_arg) {
    assert( count );

    pool *p = malloc(sizeof(pool));
    if (!p) return NULL;

    p->prm = prm_create(2, 8, 0);
    if (!p->prm) goto CLEANUP;

    p->data = pool_data_create(count);
    if (!p->data) goto CLEANUP;

    if (!pool_data_init(p, p->data, 0)) goto CLEANUP;

    p->rr        = 0;
    p->spawn     = spawn;
    p->free      = free;
    p->spawn_arg = spawn_arg;

    return p;

    CLEANUP:
    if (p->data) pool_data_free(p->data);
    if (p->prm) prm_free(p->prm);
    free(p);
    return NULL;
}

pool_data *pool_data_create(size_t count) {
    size_t size = sizeof(pool_data) + count * sizeof(pool_item *);
    pool_data *pd = malloc(size);
    if (!pd) return NULL;
    memset(pd, 0, size);

    pd->count = count;

    pd->free = bitmap_create(count);
    if (!pd->free) goto CLEANUP;

    return pd;

    CLEANUP:
    if (pd->free) bitmap_free(pd->free);

    return NULL;
}

int pool_data_init(pool *p, pool_data *pd, size_t from) {
    for (size_t i = from; i < pd->count; i++) {
        pd->items[i] = p->spawn(p->spawn_arg);
        if (!pd->items[i]) goto CLEANUP;
    }

    return 1;

    CLEANUP:
    for (size_t i = from; i < pd->count; i++) {
        if (!pd->items[i]) break;
        p->free(pd->items[i]);
    }

    return 0;
}

void pool_data_free(pool_data *pd) {
    bitmap_free(pd->free);
    free(pd);
}

pool *pool_grow(pool *p, size_t delta);

void pool_free(pool *p);

size_t pool_request(pool *p, size_t ideal, int blocking);
void   pool_release(pool *p, size_t index, int blocking);

int  pool_block  (pool *p, size_t index);
void pool_unblock(pool *p, size_t index);

void *pool_fetch(pool *p, size_t index);


