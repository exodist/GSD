#ifndef POOL_POOL_H
#define POOL_POOL_H

#include "../include/gsd_struct_types.h"
#include "../include/gsd_struct_pool.h"

typedef struct pool_item pool_item;

typedef union {
    uint64_t bundle;
    struct {
        unsigned int blocked  : 1;
        unsigned int removal  : 1;
        size_t       refcount : 62;
    } status;
} pool_item_montor;

struct pool_item {
    void *pointer;
    pool_item_montor monitor;
};

struct pool {
    size_t group_limit;
    size_t group_size;
    size_t group_index;

    size_t max_index;
    size_t index;

    size_t load_step;
    size_t usage;
    size_t blocking;

    pool_item **items;

    void *spawn_arg;
    void *(*spawn)(void *arg);
    void  (*free) (void *item);
};

int  pool_init_group(pool *p, size_t group);
void pool_term_group(pool *p, size_t group);

#endif
