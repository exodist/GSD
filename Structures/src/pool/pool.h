#ifndef POOL_POOL_H
#define POOL_POOL_H

#include "../include/gsd_struct_types.h"
#include "../include/gsd_struct_pool.h"
#include <stdint.h>

typedef struct {
    void  *item;
    uint32_t active;
    uint16_t blocks;
    uint8_t  refs;
} pool_item;

struct pool_resource {
    size_t     index;
    pool_item *item;
    uint8_t    block;
};

typedef struct {
    size_t count;

    size_t active;
    size_t blocks;

    struct pool_data *old;

    bitmap *free;
    pool_item *items[];
} pool_data;

struct pool {
    prm       *prm;
    pool_data *data;

    size_t rr;
    pool_spawn   *spawn;
    pool_unspawn *unspawn;
    void *spawn_arg;

    size_t active;
    size_t blocks;

    size_t min;
    size_t max;
    size_t load;
    size_t interval;

    uint8_t disabled;
    uint8_t resize;
};

pool_data *pool_data_create(size_t count);
void pool_data_free(pool_data *pd);

int pool_data_clone(pool *p, pool_data *from, pool_data *into);
int pool_data_init(pool *p, pool_data *pd, size_t from);

int pool_resize(pool *p, int64_t delta);

#endif
