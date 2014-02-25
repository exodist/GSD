#include "../include/gsd_struct_prm.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

int DRAN  = 0;
int DRAN2 = 0;
int DRAN3 = 0;

void destroy( void *ptr, void *arg ) {
    DRAN++;
    free( ptr );
}

void destroy2( void *ptr, void *arg ) {
    DRAN2++;
    free( ptr );
}

void destroy3( void *ptr, void *arg ) {
    DRAN3 = *((int *)arg);
    free( ptr );
}

void destroy4( void *ptr, void *arg ) {
    pthread_t *it = arg;
    *it = pthread_self();
    free( ptr );
}

void test_simple(prm *p);
void test_destructor(prm *p);
void test_multiple_users(prm *p);
void test_multi_epoch_ordered_exit(prm *p);
void test_multi_epoch_reverse_exit(prm *p);
void test_destructor_arg(prm *p);
void test_all_epochs_full_add_bag();
void test_thread_on_garbage_limit();

int main() {
    prm *p = prm_create(
        4,   // Epoch Count
        10,  // Bag limit
        100  // Fork for this much garbage
    );

    test_simple(p);
    test_destructor(p);
    test_multiple_users(p);
    test_multi_epoch_ordered_exit(p);
    test_multi_epoch_reverse_exit(p);
    test_destructor_arg(p);

    printf( "Freeing shared prm\n" );
    prm_free( p );

    test_all_epochs_full_add_bag();
    test_thread_on_garbage_limit();

    printf( "Completed all tests\n" );

    return 0;
}

void test_simple(prm *p) {
    // Join epoch
    uint8_t e = prm_join_epoch( p );
    // add garbage
    void *g = malloc(1);
    prm_dispose( p, g, NULL, NULL );
    // leave epoch
    prm_leave_epoch( p, e );

    printf( "Completed simple test\n" );
}

void test_destructor(prm *p) {
    // Join epoch
    uint8_t e = prm_join_epoch( p );
    // add garbage
    void *g = malloc(1);
    prm_dispose( p, g, destroy, NULL );
    // leave epoch
    prm_leave_epoch( p, e );
    // check that destructor ran
    assert( DRAN == 1 );

    printf( "Completed destructor test\n" );
}

void test_multiple_users(prm *p) {
    // join epoch x 2
    uint8_t e  = prm_join_epoch( p );
    uint8_t e2 = prm_join_epoch( p );
    assert( e == e2 );
    // add garbage
    void *g = malloc(1);
    prm_dispose( p, g, destroy, NULL );
    DRAN = 0;
    // leave epoch
    prm_leave_epoch( p, e );
    // check that destructor did not run
    assert( DRAN == 0 );
    // leave epoch
    prm_leave_epoch( p, e2 );
    // check that destructor ran
    assert( DRAN == 1 );

    printf( "Completed multi-user test\n" );
}

void test_multi_epoch_ordered_exit(prm *p) {
    // join epoch
    uint8_t e = prm_join_epoch( p );
    // add garbage x limit + 1. limit is 10, but destructor cuts it in half
    for ( int i = 0; i < 6; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, destroy, NULL );
    }
    // join new epoch
    uint8_t e2 = prm_join_epoch( p );
    // check that epoch changed
    assert( e != e2 );
    // add garbage
    void *g = malloc(1);
    prm_dispose( p, g, destroy2, NULL );
    // 0 out destructor counts
    DRAN  = 0;
    DRAN2 = 0;
    // leave old epoch
    prm_leave_epoch( p, e );
    // ensure first epoch garbage gone
    assert( DRAN == 5 );
    // ensure new epoch garbage remains
    assert( DRAN2 == 0 );
    // leave new epoch
    prm_leave_epoch( p, e2 );
    // ensure garbage is gone
    assert( DRAN  == 6 );
    assert( DRAN2 == 1 );

    printf( "Completed ordered epoch exit test\n" );
}

void test_multi_epoch_reverse_exit(prm *p) {
    // join epoch
    uint8_t e = prm_join_epoch( p );
    // add garbage x limit + 1. limit is 10, but destructor cuts it in half
    for ( int i = 0; i < 6; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, destroy, NULL );
    }
    // join new epoch
    uint8_t e2 = prm_join_epoch( p );
    // check that epoch changed
    assert( e != e2 );
    // add garbage
    void *g = malloc(1);
    prm_dispose( p, g, destroy2, NULL );
    // 0 out destructor counts
    DRAN  = 0;
    DRAN2 = 0;
    // leave new epoch
    prm_leave_epoch( p, e2 );
    assert( DRAN  == 0 );
    assert( DRAN2 == 0 );
    // leave old epoch
    prm_leave_epoch( p, e );
    // ensure garbage is gone
    assert( DRAN  == 6 );
    assert( DRAN2 == 1 );

    printf( "Completed reverse epoch exit test\n" );
}

void test_destructor_arg(prm *p) {
    // Join epoch
    uint8_t e = prm_join_epoch( p );
    // add garbage
    void *g = malloc(1);

    int arg = 12345;

    prm_dispose( p, g, destroy3, &arg );
    // leave epoch
    prm_leave_epoch( p, e );
    // check that destructor ran
    assert( DRAN3 == arg );

    printf( "Completed destructor arg test\n" );
}

void test_all_epochs_full_add_bag() {
    prm *p = prm_create(
        2,   // Epoch Count
        8,   // Bag limit
        100  // Fork for this much garbage
    );

    uint8_t e = prm_join_epoch( p );

    for( int i = 0; i <= 8; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, NULL, NULL );
    }

    uint8_t e2 = prm_join_epoch( p );
    assert( e != e2 );

    // This should add a bunch of bags to the second epoch
    for( int i = 0; i < 50; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, NULL, NULL );
    }

    uint8_t e3 = prm_join_epoch( p );
    assert( e2 == e3 ); // Could not change epoch

    // leave epoch
    prm_leave_epoch( p, e );
    prm_leave_epoch( p, e2 );
    prm_leave_epoch( p, e3 );
    prm_free( p );

    printf( "Completed full bag test\n" );
}

void test_thread_on_garbage_limit() {
    // Check that a new process is used when garbage is over the limit, (check
    // by storing the thread_id from the destructor).
    prm *p = prm_create(
        1,   // Epoch Count
        8,   // Bag limit
        20  // Fork for this much garbage
    );

    uint8_t e = prm_join_epoch( p );

    for( int i = 0; i <= 8; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, NULL, NULL );
    }

    // This should add a bunch of bags to the second epoch
    for( int i = 0; i < 50; i++ ) {
        void *g = malloc(1);
        prm_dispose( p, g, NULL, NULL );
    }

    pthread_t me = pthread_self();
    pthread_t it = me;
    void *g = malloc(1);
    prm_dispose( p, g, destroy4, &it );

    // leave epoch
    prm_leave_epoch( p, e );

    // This will wait for cleanup threads
    prm_free( p );

    assert( me != it ); // Make sure destructor ran in another thread

    printf( "Completed thread test\n" );
}
