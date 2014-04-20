/* vim: set filetype=c : */

#include "../include/gsd_struct_pool.h"
#include <stdio.h>
#include <assert.h>

int SPAWNED = 0;
int UNSPAWNED = 0;

void *spawn(void *arg) {
    assert(!arg);
    SPAWNED++;
    int *x = malloc(sizeof(int));
    *x = 42;
    return x;
}

void unspawn(void *item, void *arg) {
    UNSPAWNED++;
    assert(!arg);
    free(item);
}

int main() {
    SPAWNED   = 0;
    UNSPAWNED = 0;
    pool *p = pool_create(4, 8, 4, 2, spawn, unspawn, NULL);
    assert(p);
    assert(SPAWNED == 4);
    assert(UNSPAWNED == 0);
    pool_free(p);
    assert(UNSPAWNED == 4);

    SPAWNED   = 0;
    UNSPAWNED = 0;
    p = pool_create(4, 8, 4, 2, spawn, unspawn, NULL);
    assert(p);
    assert(SPAWNED == 4);

    pool_resource *res[32];
    for (int i = 0; i < 32; i++) {
        res[i] = pool_request(p, NULL, 0);
        assert(res[i]);
        int *x = pool_fetch(res[i]);
        assert(*x == 42);
    }

    assert(SPAWNED == 8);
    assert(UNSPAWNED == 0);

    for (int i = 0; i < 32; i++) {
        pool_release(p, res[i]);
    }

    assert(UNSPAWNED == 4);

    pool_free(p);
    assert(UNSPAWNED == 8);
}
