#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#include "util.h"
#include "structure.h"

uint8_t max_bit( uint64_t num ) {
    uint8_t bit = 0;
    while ( num > 0 ) {
        num >>= 1;
        bit++;
    }

    return bit;
}

rstat threaded_traverse( set *s, size_t start, size_t count, traverse_callback cb, void **args, size_t threads ) {
    size_t index = start;
    const void *arg_list[5] = { s, &index, &count, cb, args };
    rstat out = rstat_ok;

    if ( threads < 2 ) {
        rstat *ret = threaded_traverse_worker( arg_list );
        out = *ret;
        // Note: If return is an error then it will be dynamically
        // allocated memory we need to free, unless there was a
        // memory error in which case we will have a ref to
        // rstat_mem which we should not free.
        if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
    }
    else {
        pthread_t *pts = malloc( threads * sizeof( pthread_t ));
        if ( !pts ) {
            return rstat_mem;
        }
        else {
            for ( int i = 0; i < threads; i++ ) {
                pthread_create( &(pts[i]), NULL, threaded_traverse_worker, arg_list );
            }
            for ( int i = 0; i < threads; i++ ) {
                rstat *ret;
                pthread_join( pts[i], (void **)&ret );
                if ( ret->bit.error && !out.bit.error ) {
                    out = *ret;
                    // Note: If return is an error then it will be dynamically
                    // allocated memory we need to free, unless there was a
                    // memory error in which case we will have a ref to
                    // rstat_mem which we should not free.
                    if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
                }
            }
            free( pts );
        }
    }

    return out;
}

void *threaded_traverse_worker( void *in ) {
    void **arg_list = (void **)in;
    set    *set   = arg_list[0];
    size_t *index = arg_list[1];
    size_t *count = arg_list[2];

    traverse_callback *cb = arg_list[3];
    void **cb_args = arg_list[4];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= *count ) return &rstat_ok;

        rstat check = cb( set, idx, cb_args );
        if ( check.bit.error ) {
            // Allocate a new return, or return a ref to rstat_mem.
            rstat *out = malloc( sizeof( rstat ));
            if ( out == NULL ) return &rstat_mem;
            *out = check;
            return out;
        }
    }
}

