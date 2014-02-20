#include "pool.h"
#include <unistd.h>
#include <assert.h>
#include <string.h>

pool *pool_create( size_t group_size, size_t max_groups, size_t load_step, void *(*spawn)(void *arg), void (*ifree) (void *item), void *spawn_arg ) {
    pool *out = malloc(sizeof(pool));
    if (!out) return NULL;
    memset(pool, 0, sizeof(pool));

    pool->items = malloc(sizeof(pool_item*) * max_groups);
    if (!pool->items) {
        free(pool);
        return NULL;
    }
    memset(pool->items, 0, sizeof(pool_item*) * max_groups);

    pool->group_limit = max_groups;
    pool->group_size  = group_size;
    pool->group_index = 0;

    pool->max_index = max_groups * group_size;
    pool->index     = 0;

    pool->load_step = load_step;
    pool->usage     = 0;
    pool->blocking  = 0;

    pool->spawn     = spawn;
    pool->free      = ifree;
    pool->spawn_arg = arg;

    int ok = pool_init_group(out, 0);
    if (!ok) {
        free(pool->items);
        free(pool);
        return NULL;
    }

    return out;
}

int pool_init_group(pool *p, size_t group) {
}

void pool_term_group(pool *p, size_t group);

// This will block until all resources are released
void pool_free(pool *p);

// Request will try
size_t pool_request(pool *p, size_t ideal, int blocking);
void   pool_release(pool *p, size_t index, int blocking);

int  pool_block  (pool *p, size_t index);
void pool_unblock(pool *p, size_t index);

void *pool_fetch(pool *p, size_t index);


