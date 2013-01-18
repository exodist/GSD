#include "gsd_dict.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int compare( void *meta, void *key1, void *key2 ) {
    int64_t k1 = *(int64_t*)key1;
    int64_t k2 = *(int64_t*)key2;
    if ( k1 > k2 ) return -1;
    if ( k2 > k1 ) return 1;
    return 0;
}

size_t locate( void *meta, size_t slot_count, void *key ) {
    int64_t k = *(int64_t*)key;
    return k % slot_count;
}

int main() {
    dict *d;
    dict_methods *m = malloc( sizeof( dict_methods ));
    m->cmp = compare;
    m->loc = locate;
    m->del = free;

    dict_create( &d, 10, 10, NULL, m );
    return 0;
}
