#include "../include/gsd_gc.h"
#include "../gc.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

void test_all_buckets() {
}

void test_bigtag() {
}

void test_recycling() {
}

void test_return_to_system() {
}


void test_simple_no_iteration();
void test_iterator_iteration();
void test_callback_iteration();
void test_lots_of_garbage_in_epoch();
void test_destructor();

typedef struct iterator iterator;
struct iterator {
    size_t index;
    void **alloc;
};

iteration_type not_iterable( void *alloc ) { return GC_NONE; }
iteration_type use_iterator( void *alloc ) {
    uint32_t pad = gc_get_pad( alloc );
    switch( pad ) {
        case 1: return GC_NONE;
        case 2: return GC_ITERATOR;
    }
    assert( 0 );
}

void *get_iterator( void *alloc ) {
    iterator *i = malloc( sizeof( iterator ));
    i->index = 0;
    i->alloc = (void **)alloc;

    return i;
}

void *iterator_next( void *iter ) {
    iterator *i = iter;
    if ( i->index < 2 ) {
        return i->alloc[i->index++];
    }
    return NULL;
}

void iterator_free( void *iter ) {
    free( iter );
}

int main() {
    assert( sizeof(epochs) == 8 );
    assert( sizeof(tag)    == 8 );

    test_simple_no_iteration();
    test_destructor();
    test_iterator_iteration();
    test_callback_iteration();
    test_lots_of_garbage_in_epoch();

    printf( "Testing complete...\n" );
    return 0;
}

// Use valgrind for this to be useful in checking for leaks. It is useful as-is
// in checking for segfaults and other very bad things.
void test_simple_no_iteration() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
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
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, destructor_simple, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );
    start_collector( c );
    uint8_t e = gc_join_epoch( c );

    void *extra = gc_alloc( c, 1, e );
    assert( extra );

    gc_leave_epoch( c, e );

    for ( int i = 0; i < 10 && !FREED; i++ ) {
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

void test_lots_of_garbage_in_epoch() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
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

    gc_leave_epoch( c, e );

    free_collector( c );

    printf( "Completed test_lots_of_garbage_in_epoch\n" );
}

void test_iterator_iteration() {
    collector *c = build_collector(
        use_iterator,
        get_iterator,
        iterator_next,
        iterator_free,
        NULL, NULL,
        destructor_simple, NULL,
        5
    );
    void *root = gc_alloc_root( c, sizeof(void *) * 2 );
    assert( root );

    uint32_t old = 0;
    assert( gc_set_pad( root, &old, 2 ));

    start_collector( c );
    uint8_t e = gc_join_epoch( c );

    void *A = gc_alloc( c, 1, e );
    void *B = gc_alloc( c, 1, e );
    void *C = gc_alloc( c, 1, e );
    assert( A );
    assert( B );
    assert( C );
    assert( gc_set_pad( A, &old, 1 ));
    assert( gc_set_pad( B, &old, 1 ));

    // Add a and b as children of root
    void **r = (void **)root;
    r[0] = A;
    r[1] = B;

    FREED = 0;
    gc_leave_epoch( c, e );
    for ( int i = 0; i < 10 && !FREED; i++ ) {
        printf( "Waiting\n" );
        sleep(1);
    }

    // c should be cleared.
    assert( FREED == (uintptr_t)C );
    assert( gc_tag(C)->state == GC_FREE );

    assert( gc_tag(A)->state != GC_FREE );
    assert( gc_tag(B)->state != GC_FREE );

    free_collector( c );

    printf( "Completed test_iterator_iteration\n" );
}

void test_callback_iteration() {
}


