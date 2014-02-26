#ifndef GSD_STRUCT_POOL_H
#define GSD_STRUCT_POOL_H

#include <stdlib.h>
#include <stdint.h>
#include "gsd_struct_types.h"

pool *pool_create(
    size_t count, // How many items to have present

    void *(*spawn)(void *arg),  // How to create new items
    void  (*free) (void *item), // How to free items

    void *spawn_arg // Argument to spawn(void *arg)
);

pool *pool_grow(pool *p, size_t delta);

// This will block until all resources are released
void pool_free(pool *p);

size_t pool_request(pool *p, size_t ideal, int blocking);
void   pool_release(pool *p, size_t index, int blocking);

int  pool_block  (pool *p, size_t index);
void pool_unblock(pool *p, size_t index);

void *pool_fetch(pool *p, size_t index);

#endif
