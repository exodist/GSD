#include "../include/gsd_gc.h"
#include "../gc.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

void test_bigtag();
void test_recycling();
void test_return_to_system();

iteration_type not_iterable( void *alloc ) { return GC_NONE; }

int main() {
    assert( sizeof(epochs) == 8 );
    assert( sizeof(tag)    == 8 );

    test_bigtag();
    test_recycling();
    test_return_to_system();

    printf( "Testing complete...\n\n" );
    return 0;
}

void test_bigtag() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );

    start_collector_thread( c );
    uint8_t e = gc_join_epoch( c );

    void *extra1 = gc_alloc( c, 1000, e );
    assert( extra1 );

    void *extra2 = gc_alloc( c, 1000, e );
    assert( extra2 );

    gc_leave_epoch( c, e );

    stop_collector_thread( c );
    free_collector( c );

    printf( "Completed test_bigtag\n" );
}

typedef struct iterator iterator;
struct iterator {
    size_t index;
    void **alloc;
};

iteration_type use_iterator( void *alloc ) {
    uint8_t pad = gc_get_pad( alloc );
    switch( pad ) {
        case 0: return GC_NONE;
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

void test_recycling() {
    collector *c = build_collector(
        use_iterator,
        get_iterator,
        iterator_next,
        iterator_free,
        NULL,
        NULL, NULL,
        5
    );
    void *root = gc_alloc_root( c, sizeof(void *) * 2 );
    assert( root );

    uint8_t old = 0;
    assert( gc_set_pad( root, &old, 2 ));

    start_collector_thread( c );
    uint8_t e = gc_join_epoch( c );

    void *A  = gc_alloc( c, 1, e );
    void *B  = gc_alloc( c, 1, e );
    void *C1 = gc_alloc( c, 1, e );
    assert( A );
    assert( B );
    assert( C1 );
    assert( gc_set_pad( A, &old, 1 ));
    assert( gc_set_pad( B, &old, 1 ));

    // Add a and b as children of root
    void **r = (void **)root;
    r[0] = A;
    r[1] = B;

    gc_leave_epoch( c, e );
    printf( "Sleeping 4 seconds\n" );
    sleep(4);

    e = gc_join_epoch( c );
    void *C2 = gc_alloc( c, 1, e );
    printf( "A: %p\nB: %p\n", C1, C2 );
    assert( C1 == C2 );

    gc_leave_epoch( c, e );
    stop_collector_thread( c );
    free_collector( c );

    printf( "Completed test_recycling\n" );
}

void test_return_to_system() {
    collector *c = build_collector( not_iterable, NULL, NULL, NULL, NULL, NULL, NULL, 5 );
    void *root = gc_alloc_root( c, 1 );
    assert( root );
    start_collector_thread( c );
    uint8_t e = gc_join_epoch( c );

    for(int i = 0; i < 1000; i++ ) {
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

    b = c->buckets[0];
    while( b ) {
        count--;
        b = b->next;
    }

    assert( count ); // Ensure some buckets have been removed

    stop_collector_thread( c );
    free_collector( c );

    printf( "Completed test_lots_of_garbage_in_epoch\n\n" );
}


