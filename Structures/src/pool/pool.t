/* vim: set filetype=c : */

#include "../include/gsd_struct_pool.h"
#include <stdio.h>
#include <assert.h>

int SPAWNED = 0;
int UNSPAWNED = 0;

void *spawn(void *arg) {
    printf( "Test!\n" );
    //assert(!arg);
    SPAWNED++;
    return malloc(sizeof(int));
}

void unspawn(void *item, void *arg) {
    UNSPAWNED++;
    assert(!arg);
    free(item);
}

void deconstruct();

int main() {
    deconstruct();
    return 0;
}

void deconstruct() {
    SPAWNED   = 0;
    UNSPAWNED = 0;
    pool *p = pool_create(4, 8, 4, 2, spawn, unspawn, NULL);
    assert(p);
    assert(SPAWNED == 4);
    assert(UNSPAWNED == 0);
    pool_free(p);
    assert(UNSPAWNED == 4);
}
