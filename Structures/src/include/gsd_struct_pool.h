#ifndef GSD_STRUCT_POOL_H
#define GSD_STRUCT_POOL_H

#include <stdlib.h>
#include <stdint.h>
#include "gsd_struct_types.h"

typedef struct pool_resource pool_resource;

typedef void *(pool_spawn)(void *arg);
typedef void  (pool_unspawn)(void *item);

pool *pool_create(
    size_t min,      // Minimum number of resources to have available
    size_t max,      // Maximum number of resources to have available
    size_t load,     // Number of consumers per resource before resources are added
    size_t interval, // How many resources to add at a time

    pool_spawn   *spawn,   // How to create new items
    pool_unspawn *unspawn, // How to free items

    void *spawn_arg // Argument to spawn(void *arg)
);

// This will block until all resources are released
void pool_free(pool *p);

// NOTE: you must call free() on the returned resource once you are done with
// it.
pool_resource *pool_request(pool *p, pool_resource *r, int blocking);
void pool_release(pool *p, pool_resource *r);

int pool_block  (pool *p, pool_resource *r);
int pool_unblock(pool *p, pool_resource *r);

void *pool_fetch(pool *p, pool_resource *r);

#endif
