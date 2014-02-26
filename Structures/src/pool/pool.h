#ifndef POOL_POOL_H
#define POOL_POOL_H

#include "../include/gsd_struct_types.h"
#include "../include/gsd_struct_pool.h"
#include <stdint.h>

typedef struct {
    void *item;
    size_t active;
    size_t blocks;
} pool_item;

typedef struct {
    size_t count;
    bitmap *free;
    pool_item *items[];
} pool_data;

struct pool {
    prm       *prm;
    pool_data *data;

    size_t rr;
    void *(*spawn)(void *arg);
    void  (*free) (void *item);
    void *spawn_arg;

    uint8_t disabled;
};

pool_data *pool_data_create(size_t count);
void pool_data_free(pool_data *pd);

int pool_data_init(pool *p, pool_data *pd, size_t from);

#endif
