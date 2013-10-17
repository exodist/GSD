#include "../include/gsd_gc.h"
#include "../gc.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>


void test_all_buckets();
void test_bigtag();
void test_recycling();
void test_return_to_system();

iteration_type not_iterable( void *alloc ) { return GC_NONE; }

int main() {
    assert( sizeof(epochs) == 8 );
    assert( sizeof(tag)    == 8 );

    test_all_buckets();
    test_bigtag();
    test_recycling();
    test_return_to_system();

    printf( "Testing complete...\n" );
    return 0;
}

void test_all_buckets() {
}

void test_bigtag() {
}

void test_recycling() {
}

void test_return_to_system() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );
    start_collector( c );
    uint8_t e = gc_join_epoch( c );

    for(int i = 0; i < 1000; i++ ) {
        printf( "." );
        fflush( stdout );
        void *extra = gc_alloc( c, 1, e );
        assert( extra );
    }
    printf( "\n" );

    size_t count = 0;
    bucket *b = c->buckets[0];
    while( b ) {
        count++;
        b = b->next;
    }

    gc_leave_epoch( c, e );
    printf( "Sleeping 4 seconds...\n" );
    sleep(4);
    while( c->release ) {
        printf( "Sleeping 1 additional second...\n" );
        sleep(1);
    }

    b = c->buckets[0];
    while( b ) {
        count--;
        b = b->next;
    }
    
    assert( count ); // Ensure some buckets have been removed

    free_collector( c );

    printf( "Completed test_lots_of_garbage_in_epoch\n" );
}


