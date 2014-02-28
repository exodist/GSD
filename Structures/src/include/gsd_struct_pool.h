#ifndef GSD_STRUCT_POOL_H
#define GSD_STRUCT_POOL_H

#include <stdlib.h>
#include <stdint.h>
#include "gsd_struct_types.h"

typedef struct pool_resource pool_resource;

typedef void *(pool_spawn)(void *arg);
typedef void  (pool_unspawn)(void *item, void *arg);

pool *pool_create(
    int64_t min,      // Minimum number of resources to have available
    int64_t max,      // Maximum number of resources to have available
    int64_t load,     // Number of consumers per resource before resources are added
    int64_t interval, // How many resources to add at a time

    pool_spawn   *spawn,   // How to create new items
    pool_unspawn *unspawn, // How to free items

    void *spawn_arg // Argument to spawn(void *arg)
);

// This will block until all resources are released
void pool_free(pool *p);

// The value returned from pool_release is the index so that you can request
// that specific resource again if desired.
// If you pass a value into index, it will try to give you that specific
// resource. If it cannot give you that resource it will give you a different
// one, and replace the value in *index with the new resources index.
pool_resource *pool_request(pool *p, int64_t *index, int blocking);
int64_t pool_release(pool *p, pool_resource *r);

int pool_block  (pool *p, pool_resource *r);
int pool_unblock(pool *p, pool_resource *r);

void *pool_fetch(pool *p, pool_resource *r);

#endif
