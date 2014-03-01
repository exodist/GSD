#ifndef POOL_POOL_H
#define POOL_POOL_H

#include "../include/gsd_struct_types.h"
#include "../include/gsd_struct_pool.h"
#include <stdint.h>

typedef struct pool_data pool_data;

typedef struct {
    void  *item;
    size_t   refs;
    uint32_t active;
    uint16_t blocks;
} pool_item;

struct pool_resource {
    int64_t    index;
    pool_item *item;
    uint8_t    block;
};

struct pool_data {
    size_t count;
    pool_item *items[];
};

struct pool {
    prm       *prm;
    pool_data *data;
    bitmap    *free;

    size_t rr;
    pool_spawn   *spawn;
    pool_unspawn *unspawn;
    void *spawn_arg;

    size_t active;
    size_t blocks;

    int64_t min;
    int64_t max;
    int64_t load;
    int64_t interval;

    uint8_t disabled;
    uint8_t resize;
};

pool_data *pool_data_create(size_t count);
void pool_data_free(pool *p, pool_data *pd);

int pool_data_clone(pool *p, pool_data *from, pool_data *into);
int pool_data_init(pool *p, pool_data *pd, size_t from);

void pool_resize(pool *p, int64_t new_size);

void pool_balance(pool *p);

#endif
