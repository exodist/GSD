#include "pool.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"

pool *pool_create(size_t min, size_t max, size_t load, size_t iv, pool_spawn *spawn, pool_unspawn *unspawn, void *spawn_arg) {
    assert( min );
    assert( min <= max );
    assert( min == max || iv > 0 );

    pool *p = malloc(sizeof(pool));
    if (!p) return NULL;
    memset(p, 0, sizeof(pool));

    p->prm = prm_create(2, 8, 0);
    if (!p->prm) goto CLEANUP;

    p->data = pool_data_create(min);
    if (!p->data) goto CLEANUP;

    if (!pool_data_init(p, p->data, 0)) goto CLEANUP;

    p->rr        = 0;
    p->spawn     = spawn;
    p->unspawn   = unspawn;
    p->spawn_arg = spawn_arg;

    p->min  = min;
    p->max  = max;
    p->load = load;
    p->interval = iv;

    return p;

    CLEANUP:
    if (p->data) pool_data_free(p->data);
    if (p->prm)  prm_free(p->prm);
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
        pd->items[i] = malloc(sizeof(pool_item));
        if (!pd->items[i]) goto CLEANUP;

        pd->items[i]->item = p->spawn(p->spawn_arg);
        if(!pd->items[i]->item) goto CLEANUP;

        pd->items[i]->active = 0;
        pd->items[i]->blocks = 0;
        pd->items[i]->refs   = 0;
    }

    return 1;

    CLEANUP:
    for (size_t i = from; i < pd->count; i++) {
        if (!pd->items[i]) break;
        if (pd->items[i]->item) {
            p->unspawn(pd->items[i]->item);
        }
        free(pd->items[i]);
    }

    return 0;
}

void pool_data_free(pool_data *pd) {
    bitmap_free(pd->free);
    // TODO: Handle items
    free(pd);
}

int pool_data_clone(pool *p, pool_data *from, pool_data *into) {
    size_t min = from->count < into->count ? from->count : into->count;

    for(size_t i = 0; i < min; i++) {
        from->items[i]->refs++;
        into->items[i] = from->items[i];
    }

    if (into->count > min) {
        if (!pool_data_init(p, into, min)) goto CLEANUP;
    }

    return 1;

    CLEANUP:
    for(size_t i = 0; i < min; i++) {
        into->items[i]->refs--;
        into->items[i] = NULL;
    }
    return 0;
}

int pool_resize(pool *p, int64_t delta) {
    if (__atomic_load_n(&(p->disabled), __ATOMIC_CONSUME))
        return -1;

    // Atomic lock resize or return -1
    uint8_t old = __atomic_exchange_n(&(p->resize), 1, __ATOMIC_RELEASE);
    if (old) return -1; // We did not get the lock

    pool_data *new = pool_data_create(p->data->count + delta);
    if (!new) goto CLEANUP;

    int ok = pool_data_clone(p, p->data, new);
    if (!ok) goto CLEANUP;

    __atomic_store(&(new->old), &(p->data), __ATOMIC_SEQ_CST);
    __atomic_store(&(p->data), &new, __ATOMIC_SEQ_CST);
    return 1;

    CLEANUP:
    __atomic_store_n(&(p->resize), 0, __ATOMIC_SEQ_CST);
    if(new) pool_data_free(new);
    return 0;
}

void pool_free(pool *p) {
    uint8_t old = __atomic_exchange_n(&(p->disabled), 1, __ATOMIC_RELEASE);
    if (old) return;

    while(1) {
        int busy = __atomic_load_n(&(p->active), __ATOMIC_CONSUME)
            || __atomic_load_n(&(p->blocks), __ATOMIC_CONSUME);

        if (!busy) break;

        struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
        select( 0, NULL, NULL, NULL, &timeout );
    }

    // Iterate all pool_data items
}

    //if (__atomic_load_n(&(p->disabled))) return -1;
pool_resource *pool_request(pool *p, pool_resource *r, int blocking);
void pool_release(pool *p, pool_resource *r);

int pool_block  (pool *p, pool_resource *r);
int pool_unblock(pool *p, pool_resource *r);

void *pool_fetch(pool *p, pool_resource *r);


