#include "../include/gsd_gc.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

void test_simple_no_iteration();
void test_iterator_iteration();
void test_callback_iteration();
void test_lots_of_garbage_in_epoch();
void test_all_buckets();
void test_bigtag();
void test_destructor();

iteration_type not_iterable( void *alloc ) { return GC_NONE; }

int main() {
    test_simple_no_iteration();
    test_destructor();
    test_iterator_iteration();
    test_callback_iteration();
    test_lots_of_garbage_in_epoch();
    test_all_buckets();
    test_bigtag();

    printf( "Testing complete...\n" );
    return 0;
}

// Use valgrind for this to be useful in checking for leaks. It is useful as-is
// in checking for segfaults and other very bad things.
void test_simple_no_iteration() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );
    start_collector( c );
    uint8_t e = gc_join_epoch( c );

    void *extra = gc_alloc( c, 1, e );
    assert( extra );

    gc_leave_epoch( c, e );

    free_collector( c );

    printf( "Completed test_simple_no_iteration\n" );
}

uintptr_t FREED = 0;
void destructor_simple( void *alloc, void *arg ) {
    FREED = (uintptr_t)alloc;
}

void test_destructor() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, destructor_simple, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );
    start_collector( c );
    uint8_t e = gc_join_epoch( c );

    void *extra = gc_alloc( c, 1, e );
    assert( extra );

    gc_leave_epoch( c, e );

    for ( int i = 0; i < 10 && !FREED ; i++ ) {
        printf( "Waiting\n" );
        sleep(1);
    }
    assert( FREED == (uintptr_t)extra );
    FREED = 0;

    free_collector( c );
    for ( int i = 0; i < 10 && !FREED ; i++ ) {
        printf( "Waiting\n" );
        sleep(1);
    }
    assert( FREED == (uintptr_t)root );

    printf( "Completed test_destructor\n" );
}

void test_iterator_iteration() {
}

void test_callback_iteration() {
}

void test_lots_of_garbage_in_epoch() {
}

void test_all_buckets() {
}

void test_bigtag() {
}

