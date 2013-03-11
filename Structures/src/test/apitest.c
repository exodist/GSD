#include "../include/gsd_dict.h"
#include "../include/gsd_dict_return.h"
#include "structure.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

typedef struct kv kv;
struct kv {
    uint64_t  value;
    size_t    refcount;
    uint64_t  fnv_hash;
};

kv NKV = { 0, 0, 0 };

// If true FNV will be used to locate and compare keys, otherwise the raw
// integer value is used
int USE_FNV = 1;

kv    *new_kv( uint64_t val );
void   kv_ref( dict *d, void *ref, int delta );
size_t kv_loc( size_t slot_count, void *meta, void *key );
int    kv_cmp( void *meta, void *key1, void *key2 );
void   kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val );
uint64_t hash_bytes( uint8_t *data, size_t length );

kv *new_kv( uint64_t val ) {
    kv *it = malloc( sizeof( kv ));
    memset( it, 0, sizeof( kv ));
    it->refcount = 1;
    it->value = val;
    return it;
}

void kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val ) {
    return;
}

void kv_ref( dict *d, void *ref, int delta ) {
    if ( ref == NULL ) return;
    if ( ref == &NKV ) return;
    kv *r = ref;
    assert( r->refcount != 0 );
    size_t refs = __sync_add_and_fetch( &(r->refcount), delta );
    if ( refs == 0 ) free( ref );
}

size_t kv_loc( size_t slot_count, void *meta, void *key ) {
    kv *k = key;

    if ( !USE_FNV ) return k->value % slot_count;

    if ( !k->fnv_hash ) {
        k->fnv_hash = hash_bytes( (void *)&(k->value), sizeof( k->value ));
    }
    return k->fnv_hash % slot_count;
}

int kv_cmp( void *meta, void *key1, void *key2 ) {
    if ( key1 == key2 ) return 0;

    kv *k1 = key1;
    kv *k2 = key2;

    if ( USE_FNV ) {
        if ( !k1->fnv_hash ) {
            k1->fnv_hash = hash_bytes( (void *)&(k1->value), sizeof( k1->value ));
        }
        if ( !k2->fnv_hash ) {
            k2->fnv_hash = hash_bytes( (void *)&(k2->value), sizeof( k2->value ));
        }

        if ( k1->fnv_hash > k2->fnv_hash ) return -1;
        if ( k1->fnv_hash < k2->fnv_hash ) return  1;
    }

    if ( k1->value > k2->value ) return -1;
    if ( k1->value < k2->value ) return  1;

    return 0;
}

uint64_t hash_bytes( uint8_t *data, size_t length ) {
    uint64_t seed = 14695981039346656037UL;

    if ( length < 1 ) return seed;

    uint64_t key = seed;
    uint64_t i;
    for ( i = 0; i < length; i++ ) {
        key ^= data[i];
        key *= 1099511628211;
    }

    return key;
}

void test_build();
void test_create();
void test_merge();
void test_clone();
void test_meta();
void test_immute();
void test_reconf();
void test_health();
void test_balance();
void test_operations();
void test_transactions();
void test_references();
void test_triggers();
void test_iterate();
void test_sort();

dict_methods  DMET = { kv_cmp, kv_loc, kv_change, kv_ref };
dict_settings DSET = { 1024, 16, NULL };

int main() {
    test_build();
    test_create();
    test_merge();
    test_clone();
    test_meta();
    test_immute();
    test_reconf();
    test_health();
    test_balance();
    test_operations();
    test_transactions();
    test_references();
    test_triggers();
    test_iterate();
    test_sort();

    return 0;
}

void test_build() {
    dict *d = dict_build( 1024, DMET, NULL );
    assert( d );

    kv *it = new_kv( 10 );
    dict_stat s = dict_insert( d, it, it );
    if ( s.bit.error ) {
        fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        exit( 1 );
    }
    kv_ref( d, it, -1 );

    dict_free( &d );
    assert( !d );
}

void test_create() {
    dict *d = NULL;
    rstat c = dict_create( &d, DSET, DMET );
    assert( !c.bit.error );
    assert( d );

    kv *it = new_kv( 10 );
    dict_stat s = dict_insert( d, it, it );
    if ( s.bit.error ) {
        fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        exit( 1 );
    }
    kv_ref( d, it, -1 );

    dict_free( &d );
    assert( !d );
}

dict *init_dict_20() {
    dict *d = dict_build( 1024, DMET, NULL );
    assert( d );

    for ( int i = 0; i <= 20; i++ ) {
        kv *it = new_kv( i );
        dict_stat s = dict_insert( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        kv_ref( d, it, -1 );
    }

    return d;
}

void test_merge_insert() {
    kv *k = NULL;
    kv *v = NULL;
    kv *got = NULL;
    dict *d1 = init_dict_20();
    dict *d2 = dict_build( 1024, DMET, NULL );
    dict_stat s = { 0 };

    // Set #20 in d2 so it is set before merge
    k = new_kv( 20 );
    v = new_kv( 99 );
    dict_set( d2, k, v );
    kv_ref( d1, k, -1 );
    kv_ref( d1, v, -1 );

    dict_merge_settings mset = { MERGE_INSERT, 0 };
    dict_merge( d1, d2, mset, 8 );

    // Check that merge succeded
    for ( int i = 0; i < 20; i++ ) {
        k = new_kv( i );
        v = NULL;
        s = dict_get( d2, k, (void **)&v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        assert( v->value == i );
        kv_ref( d2, k, -1 );
        kv_ref( d2, v, -1 );
    }

    // Check that #20 did NOT get updated
    got = NULL;
    k = new_kv( 20 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 99 );
    kv_ref( d1, k,   -1 );
    kv_ref( d1, got, -1 );

    // Change d1 and ensure d2 is not also changed.
    k = new_kv( 10 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    dict_get( d1, k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 10 );
    kv_ref( d1, got, -1 );
    kv_ref( d1, k,   -1 );
    kv_ref( d1, v,   -1 );

    // Check references
    dict_free( &d1 );
    dict_free( &d2 );
    d1 = init_dict_20();
    d2 = dict_build( 1024, DMET, NULL );

    mset.reference = 1;
    dict_merge( d1, d2, mset, 8 );
    // Check that merge succeded
    for ( int i = 0; i < 20; i++ ) {
        k = new_kv( i );
        v = NULL;
        s = dict_get( d2, k, (void **)&v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        assert( v->value == i );
        kv_ref( d2, k, -1 );
        kv_ref( d2, v, -1 );
    }

    // Change d1 and ensure d2 reflects the change
    k = new_kv( 10 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    got = NULL;
    dict_get( d1,  k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d2, got, -1 );

    kv_ref( d1, k, -1 );
    kv_ref( d1, v, -1 );

    dict_free( &d1 );
    dict_free( &d2 );
}

void test_merge_update() {
    kv *k = NULL;
    kv *v = NULL;
    kv *got = NULL;
    dict *d1 = init_dict_20();
    //dict *d1 = dict_build( 1, DMET, NULL );
    dict *d2 = dict_build( 1, DMET, NULL );
    dict_stat s = { 0 };

    // Set #20 in d2 so it is set before merge
    k = new_kv( 20 );
    v = new_kv( 99 );
    dict_set( d1, k, k );
    dict_set( d2, k, v );
    kv_ref( d2, k, -1 );
    kv_ref( d2, v, -1 );

    dict_merge_settings mset = { MERGE_UPDATE, 0 };
    dict_merge( d1, d2, mset, 8 );

    // Check that merge ignored keys not already set in dest
    for ( int i = 0; i < 20; i++ ) {
        k = new_kv( i );
        v = NULL;
        s = dict_get( d2, k, (void **)&v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        assert( v == NULL );
        kv_ref( d2, k, -1 );
    }

    // Check that #20 did get updated
    got = NULL;
    k = new_kv( 20 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 20 );
    kv_ref( d2, k,   -1 );
    kv_ref( d2, got, -1 );

    // Change d1 and ensure d2 is not also changed.
    k = new_kv( 20 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    dict_get( d1, k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 20 );
    kv_ref( d1, got, -1 );
    kv_ref( d1, k,   -1 );
    kv_ref( d1, v,   -1 );

    // Check references
    dict_free( &d1 );
    dict_free( &d2 );
    d1 = init_dict_20();
    d2 = dict_build( 1024, DMET, NULL );

    // Set #20 in d2 so it is set before merge
    k = new_kv( 20 );
    v = new_kv( 99 );
    dict_set( d2, k, v );
    kv_ref( d2, k, -1 );
    kv_ref( d2, v, -1 );

    mset.reference = 1;
    s = dict_merge( d1, d2, mset, 8 );
    if ( s.bit.error ) {
        fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        exit( 1 );
    }

    // Change d1 and ensure d2 reflects the change
    k = new_kv( 20 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    got = NULL;
    dict_get( d1, k, (void **)&got );
    assert( got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got );
    assert( got->value == 100 );
    kv_ref( d2, got, -1 );

    kv_ref( d1, k, -1 );
    kv_ref( d1, v, -1 );

    dict_free( &d1 );
    dict_free( &d2 );
}

void test_merge_set() {
    kv *k = NULL;
    kv *v = NULL;
    kv *got = NULL;
    dict *d1 = init_dict_20();
    dict *d2 = dict_build( 1024, DMET, NULL );
    dict_stat s = { 0 };

    // Set #20 in d2 so it is set before merge
    k = new_kv( 20 );
    v = new_kv( 99 );
    dict_set( d2, k, v );
    kv_ref( d1, k, -1 );
    kv_ref( d1, v, -1 );

    dict_merge_settings mset = { MERGE_SET, 0 };
    dict_merge( d1, d2, mset, 8 );

    // Check that merge succeded
    for ( int i = 0; i <= 20; i++ ) {
        k = new_kv( i );
        v = NULL;
        s = dict_get( d2, k, (void **)&v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        assert( v->value == i );
        kv_ref( d2, k, -1 );
        kv_ref( d2, v, -1 );
    }

    // Check that #20 did get updated
    got = NULL;
    k = new_kv( 20 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 20 );
    kv_ref( d1, k,   -1 );
    kv_ref( d1, got, -1 );

    // Change d1 and ensure d2 is not also changed.
    k = new_kv( 10 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    dict_get( d1, k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 10 );
    kv_ref( d1, got, -1 );
    kv_ref( d1, k,   -1 );
    kv_ref( d1, v,   -1 );

    // Check references
    dict_free( &d1 );
    dict_free( &d2 );
    d1 = init_dict_20();
    d2 = dict_build( 1024, DMET, NULL );

    mset.reference = 1;
    dict_merge( d1, d2, mset, 8 );
    // Check that merge succeded
    for ( int i = 0; i < 20; i++ ) {
        k = new_kv( i );
        v = NULL;
        s = dict_get( d2, k, (void **)&v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            exit( 1 );
        }
        assert( v->value == i );
        kv_ref( d2, k, -1 );
        kv_ref( d2, v, -1 );
    }

    // Change d1 and ensure d2 reflects the change
    k = new_kv( 10 );
    v = new_kv( 100 );
    dict_update( d1, k, v );
    if ( s.bit.error ) {
        fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        exit( 1 );
    }
    got = NULL;
    dict_get( d1,  k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d1, got, -1 );
    dict_get( d2, k, (void **)&got );
    assert( got->value == 100 );
    kv_ref( d2, got, -1 );

    kv_ref( d1, k, -1 );
    kv_ref( d1, v, -1 );

    dict_free( &d1 );
    dict_free( &d2 );
}

void test_merge() {
    test_merge_insert();
    test_merge_update();
    test_merge_set();
}

void test_clone() {
    dict *d1 = NULL;
    dict *d2 = NULL;

    kv *k = new_kv( 10 );
    kv *v = new_kv( 99 );
    kv *g = NULL;

    // Clone no refs
    d1 = init_dict_20();
    d2 = dict_clone( d1, 0, 8 );
    assert( d2 );

    // Check clone
    dict_get( d2, k, (void **)&g );
    assert( g );
    assert( g->value == 10 );
    kv_ref( d2, g, -1 );

    // Set original
    dict_set( d1, k, v );

    // Check that clone does not see change
    dict_get( d2, k, (void **)&g );
    assert( g->value == 10 );
    kv_ref( d2, g, -1 );

    // Clone with refs
    dict_free( &d1 );
    dict_free( &d2 );
    d1 = init_dict_20();
    d2 = dict_clone( d1, 1, 8 );
    assert( d2 );

    // Check clone
    dict_get( d2, k, (void **)&g );
    assert( g );
    assert( g->value == 10 );
    kv_ref( d2, g, -1 );

    // Set original
    dict_set( d1, k, v );

    // Check that clone sees change
    dict_get( d2, k, (void **)&g );
    assert( g->value == 99 );
    kv_ref( d2, g, -1 );

    // Cleanup
    kv_ref( d2, k, -1 );
    kv_ref( d2, v, -1 );
    dict_free( &d1 );
    dict_free( &d2 );
}

void test_meta() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // get_settings
    // get_methods

    dict_free( &d );
}

void test_immute() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // clone immutable
    // attempt to modify

    dict_free( &d );
}

void test_reconf() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Reconfigure

    dict_free( &d );
}

void test_health() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Fake an invalid state
    // Health check
    // recover

    dict_free( &d );
}

void test_balance() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Make very imbalanced tree
    // rebalance

    dict_free( &d );
}

void test_operations() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Insert success
    // Insert fail
    // update success
    // update fail
    // set
    // delete success
    // delete fail
    // get success
    // get fail

    dict_free( &d );
}

void test_transactions() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // cmp_update
    // cmp_delete

    dict_free( &d );
}

void test_references() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Make reference
    // Change+test reference
    // dereference

    dict_free( &d );
}

void test_triggers() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // insert trigger
    // give good value
    // give bad value

    dict_free( &d );
}

void test_iterate() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Make sure all inserted key+value pairs are hit.
    // Probably use 2 arrays, keys and values, set all to all 0's then have
    // iteration callback set to 1 for each array as encountered.

    dict_free( &d );
}

void test_sort() {
    dict *d = dict_build( 1024, DMET, NULL );

    // TODO:
    // Make array with 1 slot, and a non fnv sort, make sure iterate gives us
    // sorted order.

    dict_free( &d );
}
