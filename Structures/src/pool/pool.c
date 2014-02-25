#include "pool.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>

pool *pool_create(size_t group_size, size_t max_groups, size_t load_step, void *(*spawn)(void *arg), void (*ifree) (void *item), void *spawn_arg) {
    pool *out = malloc(sizeof(pool));
    if (!out) return NULL;
    memset(out, 0, sizeof(pool));

    out->items = malloc(sizeof(pool_item**) * max_groups);
    if (!out->items) {
        free(out);
        return NULL;
    }
    memset(out->items, 0, sizeof(pool_item**) * max_groups);

    out->group_limit = max_groups;
    out->group_size  = group_size;
    out->group_index = 0;

    out->max_index = max_groups * group_size;
    out->index     = 0;

    out->load_step = load_step;
    out->usage     = 0;
    out->blocking  = 0;

    out->spawn     = spawn;
    out->free      = ifree;
    out->spawn_arg = spawn_arg;

    int ok = pool_init_group(out, 0);
    if (!ok) {
        free(out->items);
        free(out);
        return NULL;
    }

    return out;
}

int pool_init_group(pool *p, size_t group) {
    pool_item *current = NULL;
    __atomic_load(p->items + group, &current, __ATOMIC_CONSUME);
    if (current) return -1;

    void *new = malloc(sizeof(pool_item *) * p->group_size);
    if (!new) return 0;
    memset(new, 0, sizeof(pool_item *) * p->group_size);

    while(!current) {
        int ok = __atomic_compare_exchange(
            p->items + group,
            &current,
            &new,
            0,
            __ATOMIC_ACQ_REL,
            __ATOMIC_CONSUME
        );
        if (ok) {
            __atomic_store_n(&(p->index), group * p->group_size, __ATOMIC_ACQ_REL);
            return 1;
        }
    }

    free(new);
    return -1;
}

// This will block until all resources are released
void pool_free(pool *p);

// Request will try
size_t pool_request(pool *p, size_t ideal, int blocking);
void   pool_release(pool *p, size_t index, int blocking);

int  pool_block  (pool *p, size_t index);
void pool_unblock(pool *p, size_t index);

void *pool_fetch(pool *p, size_t index);


