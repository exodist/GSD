#include "../include/gsd_prm.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#define THREADS 16
#define ALLOCS  1000

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

void do_it();

int main() {
    for (int i = 1; i <= 1000; i++ ) {
        do_it();
        printf( "%-7i", i );
        if (!(i % 10)) printf( "\n" );
        fflush( stdout );
    }

    printf( "\nCompleted all tests\n" );

    return 0;
}

void do_it() {
    prm *p = build_prm(
        4,   // Epoch Count
        64, // Bag limit
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

    for (int t = 0; t < THREADS; t++) {
        pthread_join( pthreads[t], NULL );
        for (int i = 0; i < ALLOCS; i++) {
            // Should be set, but not destroyed
            assert( *(allocs[t][i]) == 25 );
        }
    }

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
    leave_epoch( p, e );

    free_prm( p );

    for (int i = 0; i < THREADS; i++ ) {
        free(allocs[i]);
    }
    free(allocs);
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
