#include "../include/gsd_prm.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define THREADS 16
#define ALLOCS  10000

void *worker( void * );

typedef struct worker_args worker_args;
struct worker_args {
    prm      *p;
    uint8_t **allocs;
};

void destroy1(void *ptr, void *arg) {
    uint8_t *p = ptr;
    assert( *p == 25 );
    *p = 30;
}

void destroy2(void *ptr, void *arg) {
    uint8_t *p = ptr;
    assert( *p == 35 );
    free( p );
}

int main() {
    prm *p = build_prm(
        4,   // Epoch Count
        100, // Bag limit
        0    // Fork for this much garbage
    );

    uint8_t e = join_epoch( p );

    // threads x allocs
    uint8_t ***allocs = malloc( sizeof(uint8_t **) * THREADS );
    assert( allocs );
    for (int i = 0; i < THREADS; i++ ) {
        allocs[i] = malloc(sizeof(uint8_t *) * ALLOCS);
        assert( allocs[i] );
    }

    pthread_t pthreads[THREADS];
    for (int t = 0; t < THREADS; t++) {
        worker_args *args = malloc( sizeof( worker_args ));
        args->p      = p;
        args->allocs = allocs[t];
        pthread_create( pthreads + t, NULL, worker, args );
    }
    printf( "Threads initialized\n" );

    for (int t = 0; t < THREADS; t++) {
        pthread_join( pthreads[t], NULL );
        for (int i = 0; i < ALLOCS; i++) {
            // Should be set, but not destroyed
            assert( *(allocs[t][i]) == 25 );
        }
    }
    printf( "Threads completed\n" );

    leave_epoch( p, e );

    e = join_epoch( p );
    for (int t = 0; t < THREADS; t++) {
        for (int i = 0; i < ALLOCS; i++) {
            // Should be destroyed
            assert( *(allocs[t][i]) == 30 );
            *(allocs[t][i]) = 35;
            dispose( p, allocs[t][i], destroy2, NULL );
        }
    }
    printf( "Disposal completed\n" );
    leave_epoch( p, e );

    printf( "Freeing shared prm\n" );
    free_prm( p );
    printf( "Completed all tests\n" );

    for (int i = 0; i < THREADS; i++ ) {
        free(allocs[i]);
    }
    free(allocs);

    return 0;
}

void *worker( void *in ) {
    worker_args *args = in;
    for ( int i = 0; i < ALLOCS; i++ ) {
        uint8_t e = join_epoch( args->p );
        args->allocs[i] = malloc( sizeof( uint8_t ));
        *(args->allocs[i]) = 25;
        dispose( args->p, args->allocs[i], destroy1, NULL );
        leave_epoch( args->p, e );
    }

    free( in );
    return NULL;
}
