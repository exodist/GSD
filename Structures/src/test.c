#define DICT_DEBUG "monitor.dot"

#include "gsd_dict_api.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int compare( void *meta, void *key1, void *key2 ) {
    int64_t k1 = *(int64_t*)key1;
    int64_t k2 = *(int64_t*)key2;
    if ( k1 > k2 ) return -1;
    if ( k2 > k1 ) return 1;
    return 0;
}

char *show( void *key, void *val ) {
    int64_t k = key ? *(int64_t*)key : -1;
    int64_t v = val ? *(int64_t*)val : -1;
    char *buffer = malloc( 20 );
    snprintf( buffer, 20, "%li:%li", k, v );
    return buffer;
}

size_t locate( void *meta, size_t slot_count, void *key ) {
    int64_t k = *(int64_t*)key;
    int64_t s = k % slot_count;
    return s;
}

int main() {
    srand(time(NULL));

    dict *d;
    dict_methods *m = malloc( sizeof( dict_methods ));
    m->cmp = compare;
    m->loc = locate;

    dict_create( &d, 1, 10, NULL, m );
    int64_t v = 1;
    int64_t v2 = 11;
    int64_t k[1001];
    for (int i = 0; i < 1001; i++) {
        k[i] = i;
    }

    for ( int i = 0; i < 500; i += 5 ) {
        int x = rand() % 100;
        dict_set( d, &k[x], &v );
        if ( i % 25 == 0 ) {
            dict_delete( d, &k[x] );
        }
    }

    dict_set( d, &k[5], &v );
    dict_set( d, &k[8], &v );
    dict_delete( d, &k[5] );
    dict_delete( d, &k[8] );
    dict_set( d, &k[8], &v2 );

    char *buffer;
    int ret = dict_dump_dot( d, &buffer, show );
    if ( ret ) { printf( "Error\n" ); }

    printf( "%s\n", buffer );

    return ret;
}
