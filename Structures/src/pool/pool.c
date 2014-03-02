#include "pool.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include "../include/gsd_struct_bitmap.h"
#include "../include/gsd_struct_prm.h"

pool *pool_create(int64_t min, int64_t max, int64_t load, int64_t iv, pool_spawn *spawn, pool_unspawn *unspawn, void *spawn_arg) {
    assert( min > 0 );
    assert( min <= max );
    assert( min == max || iv > 0 );

    pool *p = malloc(sizeof(pool));
    if (!p) return NULL;
    memset(p, 0, sizeof(pool));

    p->prm = prm_create(2, 8, 0);
    if (!p->prm) goto CLEANUP;

    p->data = pool_data_create(min);
    if (!p->data) goto CLEANUP;

    p->free = bitmap_create(max);
    if (!p->free) goto CLEANUP;

    p->spawn     = spawn;
    p->unspawn   = unspawn;
    p->spawn_arg = spawn_arg;

    p->min  = min;
    p->max  = max;
    p->load = load;
    p->interval = iv;

    if (!pool_data_init(p, p->data, 0)) goto CLEANUP;

    return p;

    CLEANUP:
    if (p->data) free(p->data);
    if (p->free) bitmap_free(p->free);
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

    return pd;
}

int pool_data_init(pool *p, pool_data *pd, size_t from) {
    for (size_t i = from; i < pd->count; i++) {
        pd->items[i] = malloc(sizeof(pool_item));
        if (!pd->items[i]) goto CLEANUP;

        assert(p->spawn);
        pd->items[i]->item = p->spawn(p->spawn_arg);
        if(!pd->items[i]->item) goto CLEANUP;

        pd->items[i]->active = 0;
        pd->items[i]->blocks = 0;
        pd->items[i]->refs   = 1;
    }

    return 1;

    CLEANUP:
    for (size_t i = from; i < pd->count; i++) {
        if (!pd->items[i]) break;
        if (pd->items[i]->item) {
            p->unspawn(pd->items[i]->item, p->spawn_arg);
        }
        free(pd->items[i]);
    }

    return 0;
}

void pool_data_destroy(void *pd, void *p) {
    pool_data_free(p, pd);
}

void pool_data_free(pool *p, pool_data *pd) {
    for (size_t i = 0; i < pd->count; i++) {
        if (!pd->items[i]) continue;
        size_t refs = __atomic_sub_fetch(&(pd->items[i]->refs), 1, __ATOMIC_ACQ_REL);
        if (!refs) {
            uint8_t e = prm_join_epoch(p->prm);
            prm_dispose(p->prm, pd->items[i]->item, p->unspawn, p->spawn_arg);
            prm_dispose(p->prm, pd->items[i], NULL, NULL);
            prm_leave_epoch(p->prm, e);
        }
        pd->items[i] = NULL;
    }

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

void pool_resize(pool *p, int64_t new_size) {
    if (__atomic_load_n(&(p->disabled), __ATOMIC_CONSUME))
        return;

    pool_data *new = pool_data_create(new_size);
    if (!new) return;

    int ok = pool_data_clone(p, p->data, new);
    if (!ok) {
        pool_data_free(p, new);
        return;        
    };

    uint8_t e = prm_join_epoch(p->prm);

    pool_data *old_pd = p->data;

    __atomic_store(&(p->data), &new, __ATOMIC_SEQ_CST);

    prm_dispose(p->prm, old_pd, pool_data_destroy, p);

    prm_leave_epoch(p->prm, e);
}

void pool_free(pool *p) {
    uint8_t old = __atomic_exchange_n(&(p->disabled), 1, __ATOMIC_RELEASE);
    if (old) return;

    uint8_t e = prm_join_epoch(p->prm);
    while (1) {
        uint8_t old = __atomic_exchange_n(&(p->resize), 1, __ATOMIC_RELEASE);
        if (!old) break;

        struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
        select( 0, NULL, NULL, NULL, &timeout );
    }

    while(1) {
        int busy = __atomic_load_n(&(p->active), __ATOMIC_CONSUME)
            || __atomic_load_n(&(p->blocks), __ATOMIC_CONSUME);

        if (!busy) break;

        struct timeval timeout = { .tv_sec = 0, .tv_usec = 100 };
        select( 0, NULL, NULL, NULL, &timeout );
    }

    pool_data *pd = NULL;
    __atomic_load( &(p->data), &pd, __ATOMIC_ACQUIRE);

    if (pd) prm_dispose(p->prm, pd, pool_data_destroy, p);

    prm_leave_epoch(p->prm, e);
    prm_free(p->prm);
    bitmap_free(p->free);
    free(p);
}

pool_resource *pool_request(pool *p, int64_t *index, int blocking) {
    if (__atomic_load_n(&(p->disabled), __ATOMIC_ACQUIRE)) return NULL;

    pool_resource *out = malloc(sizeof(pool_resource));
    if (!out) return NULL;
    memset(out, 0, sizeof(pool_resource));

    uint8_t e = prm_join_epoch(p->prm);

    pool_data *pd = NULL;
    __atomic_load( &(p->data), &pd, __ATOMIC_ACQUIRE);
    if (!pd) goto CLEANUP;

    int64_t ridx = -1;
    if (index && *index < pd->count) {
        if (blocking && pd->items[*index]->blocks)   ridx = *index;
        if (!blocking && !pd->items[*index]->blocks) ridx = *index;
    }
    if (ridx < 0) {
        ridx = bitmap_fetch(p->free, 0, pd->count);
    }
    if (ridx < 0) {
        if (blocking && p->blocks >= p->max / 3) {
            size_t initial = __atomic_fetch_add(&(p->brr), 1, __ATOMIC_ACQ_REL) % pd->count;
            ridx = initial;
            while (!pd->items[ridx]->blocks) {
                ridx = __atomic_fetch_add(&(p->brr), 1, __ATOMIC_ACQ_REL) % pd->count;
                // full loop; In case blocks went from max to none in the time we took to do this.
                if (ridx == initial) break;
            }
        }
        else {
            ridx = __atomic_fetch_add(&(p->rr), 1, __ATOMIC_ACQ_REL) % pd->count;
        }
    }

    assert(ridx >= 0);

    if (index) *index = ridx;

    out->index = ridx;
    out->item  = pd->items[ridx];

    if (blocking) {
        int ok = pool_block(p, out);
        if (ok < 1) goto CLEANUP;
    }

    assert(__atomic_fetch_add(&(out->item->refs),   1, __ATOMIC_ACQ_REL));

    // There is a bit of a race condition here. It is possible an RR would be
    // at the same index as a free one. In such a case 2 threads would try to
    // obtain the resource at the same time, and it is not clear which one will
    // be setting the free bitmap. This is a harmless race condition as both
    // threads would still use the resource even if there was no race
    // condition.
    size_t active = __atomic_add_fetch(&(out->item->active), 1, __ATOMIC_ACQ_REL);
    if (active == 1) bitmap_set(p->free, out->index, 1);

    assert(__atomic_add_fetch(&(p->active), 1, __ATOMIC_ACQ_REL));

    prm_leave_epoch(p->prm, e);

    pool_balance(p);

    return out;

    CLEANUP:
    prm_leave_epoch(p->prm, e);
    free(out);
    return NULL;
}

int64_t pool_release(pool *p, pool_resource *r) {
    int64_t idx = r->index;

    uint8_t e = prm_join_epoch(p->prm);

    if(r->block) pool_unblock(p, r); 

    __atomic_sub_fetch(&(p->active), 1, __ATOMIC_ACQ_REL);
    size_t refs = __atomic_sub_fetch(&(r->item->refs), 1, __ATOMIC_ACQ_REL);
    if (!refs) {
        assert(!__atomic_sub_fetch(&(r->item->active), 1, __ATOMIC_ACQ_REL));
        prm_dispose(p->prm, r->item->item, p->unspawn, p->spawn_arg);
        prm_dispose(p->prm, r->item, NULL, NULL);
    }
    else {
        // Same race condition as in request, it is harmless.
        size_t active = __atomic_sub_fetch(&(r->item->active), 1, __ATOMIC_ACQ_REL);
        if (!active) bitmap_set(p->free, r->index, 0);
    }

    free(r);

    prm_leave_epoch(p->prm, e);

    pool_balance(p);

    return idx;
}

int pool_block(pool *p, pool_resource *r) {
    uint8_t old = __atomic_exchange_n(&(r->block), 1, __ATOMIC_RELEASE);
    if (old) return -1;

    assert(__atomic_add_fetch(&(r->item->blocks), 1, __ATOMIC_ACQ_REL));
    assert(__atomic_add_fetch(&(p->blocks), 1, __ATOMIC_ACQ_REL));

    return 1;
}

int pool_unblock(pool *p, pool_resource *r) {
    if (!__atomic_load_n(&(r->block), __ATOMIC_CONSUME))
        return 0;

    assert(__atomic_sub_fetch(&(r->item->blocks), 1, __ATOMIC_ACQ_REL));
    assert(__atomic_sub_fetch(&(p->blocks), 1, __ATOMIC_ACQ_REL));

    assert(__atomic_exchange_n(&(r->block), 0, __ATOMIC_RELEASE));

    return 1;
}

void *pool_fetch(pool *p, pool_resource *r) {
    if (!r->item) return NULL;
    return r->item->item;
}

void pool_balance(pool *p) {
    // Lock resizing
    uint8_t old = __atomic_exchange_n(&(p->resize), 1, __ATOMIC_RELEASE);
    if (old) return; // We did not get the lock

    size_t blocks = __atomic_load_n(&(p->blocks), __ATOMIC_CONSUME);
    size_t active = __atomic_load_n(&(p->active), __ATOMIC_CONSUME);

    size_t usage = (blocks * p->load) + active;

    // No need to atomic_load p->data, we are the only thread that can change it.
    size_t load = usage / p->data->count;
    size_t over = usage % p->data->count;

    if (load >= p->load) {
        if (over && p->data->count < p->max) {
            int64_t new_size = p->data->count + p->interval;
            new_size = new_size > p->max ? p->max : new_size;
            pool_resize(p, new_size);
        }
    }
    else if (load < p->load && p->data->count > p->min) {
        size_t count = p->data->count - p->interval;
        count = count < p->min ? p->min : count;
        assert(count);

        // Calc new usage if INTERVAL consumers were to join. We do not want to
        // shrink if we have to grow again too soon.
        size_t new_load = usage + p->interval / count;

        if (new_load < p->load) {
            int64_t new_size = p->data->count - p->interval;
            new_size = new_size > p->max ? p->min : new_size;
            pool_resize(p, new_size);
        }
    }

    assert(__atomic_exchange_n(&(p->resize), 0, __ATOMIC_SEQ_CST));
}
