#include "../include/gsd_prm.h"
#include <stdio.h>
#include <assert.h>

int DRAN  = 0;
int DRAN2 = 0;

void destroy( void *ptr ) {
    DRAN++;
    free( ptr );
}

void destroy2( void *ptr ) {
    DRAN2++;
    free( ptr );
}

void test_simple(prm *p);
void test_destructor(prm *p);
void test_multiple_users(prm *p);
void test_multi_epoch_ordered_exit(prm *p);
void test_multi_epoch_reverse_exit(prm *p);

int main() {
    prm *p = build_prm(
        4,   // Epoch Count
        10,  // Bag limit
        100  // Fork for this much garbage
    );

    test_simple(p);
    test_destructor(p);
    test_multiple_users(p);
    test_multi_epoch_ordered_exit(p);
    test_multi_epoch_reverse_exit(p);

    printf( "Freeing prm\n" );
    free_prm( p );

    printf( "No errors!\n" );

    return 0;
}

void test_simple(prm *p) {
    // Join epoch
    uint8_t e = join_epoch( p );
    // add garbage
    void *g = malloc(1);
    dispose( p, g, NULL );
    // leave epoch
    leave_epoch( p, e );
}

void test_destructor(prm *p) {
    // Join epoch
    uint8_t e = join_epoch( p );
    // add garbage
    void *g = malloc(1);
    dispose( p, g, destroy );
    // leave epoch
    leave_epoch( p, e );
    // check that destructor ran
    assert( DRAN == 1 );
}

void test_multiple_users(prm *p) {
    // join epoch x 2
    uint8_t e  = join_epoch( p );
    uint8_t e2 = join_epoch( p );
    assert( e == e2 );
    // add garbage
    void *g = malloc(1);
    dispose( p, g, destroy );
    DRAN = 0;
    // leave epoch
    leave_epoch( p, e );
    // check that destructor did not run
    assert( DRAN == 0 );
    // leave epoch
    leave_epoch( p, e2 );
    // check that destructor ran
    assert( DRAN == 1 );
}

void test_multi_epoch_ordered_exit(prm *p) {
    // join epoch
    uint8_t e = join_epoch( p );
    // add garbage x limit + 1. limit is 10, but destructor cuts it in half
    for ( int i = 0; i < 6; i++ ) {
        void *g = malloc(1);
        dispose( p, g, destroy );
    }
    // join new epoch
    uint8_t e2 = join_epoch( p );
    // check that epoch changed
    assert( e != e2 );
    // add garbage
    void *g = malloc(1);
    dispose( p, g, destroy2 );
    // 0 out destructor counts
    DRAN  = 0;
    DRAN2 = 0;
    // leave old epoch
    leave_epoch( p, e );
    // ensure first epoch garbage gone
    assert( DRAN == 5 );
    // ensure new epoch garbage remains
    assert( DRAN2 == 0 );
    // leave new epoch
    leave_epoch( p, e2 );
    // ensure garbage is gone
    assert( DRAN  == 6 );
    assert( DRAN2 == 1 );
}

void test_multi_epoch_reverse_exit(prm *p) {
    // join epoch
    uint8_t e = join_epoch( p );
    // add garbage x limit + 1. limit is 10, but destructor cuts it in half
    for ( int i = 0; i < 6; i++ ) {
        void *g = malloc(1);
        dispose( p, g, destroy );
    }
    // join new epoch
    uint8_t e2 = join_epoch( p );
    // check that epoch changed
    assert( e != e2 );
    // add garbage
    void *g = malloc(1);
    dispose( p, g, destroy2 );
    // 0 out destructor counts
    DRAN  = 0;
    DRAN2 = 0;
    // leave new epoch
    leave_epoch( p, e2 );
    assert( DRAN  == 0 );
    assert( DRAN2 == 0 );
    // leave old epoch
    leave_epoch( p, e );
    // ensure garbage is gone
    assert( DRAN  == 6 );
    assert( DRAN2 == 1 );
}

void test_all_epochs_full_add_bag() {

}

void test_fork_on_garbage_limit() {
    // Check that a new process is used when garbage is over the limit, (check
    // by storing the thread_id from the destructor).
}
