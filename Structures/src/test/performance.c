#include "../include/gsd_dict.h"
#include "../include/gsd_dict_return.h"
#include "structure.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

typedef struct timespec timespec;
typedef struct kv kv;
struct kv {
    uint64_t value;
    char     cat;
    size_t   refcount;
    uint64_t fnv_hash;
};

kv NKV = { 1, 'N', 0, 0 };

uint64_t hash_bytes( uint8_t *data, size_t length );

void   kv_ref( dict *d, void *ref, int delta );
size_t kv_loc( dict_settings *s, void *key );
int    kv_cmp( dict_settings *s, void *key1, void *key2 );
void   kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val );

kv *new_kv( char cat, uint64_t val );

void *thread_do_inserts( void *ptr );
void *thread_do_lookups( void *ptr );

typedef struct threadargs threadargs;
struct threadargs {
    dict *dict;
    int  operations;
    int  id;
};

timespec time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

int main() {
    dict_settings set = { 16, 0, NULL };
    dict_methods  met = { kv_cmp, kv_loc, kv_change, kv_ref };
    dict *d = NULL;

    int operations = 100000;
    int min_slots = 4;
    int max_slots = 512;
    int max_imbalance = 512;
    int min_imbalance = 4;
    int min_threads = 1;
    int max_threads = 8;
    int min_epochs = 16;
    int max_epochs = 512;

    fprintf( stdout,
        "Type, "
        "Max Epochs, "
        "Threads, "
        "Slots, "
        "Max Imbalance, "
        "Operations, "
        "Time, "
        "Rebalanced, "
        "Used Epochs\n"
    );
    fflush( stdout );

    for ( int epochs = min_epochs; epochs <= max_epochs; epochs *= 2 ) {
        for ( int threads = min_threads; threads <= max_threads; threads *= 2 ) {
            for ( int slots = min_slots; slots <= max_slots; slots *= 2 ) {
                for ( int imbalance = min_imbalance; imbalance <= max_imbalance; imbalance *= 2 ) {
                    set.max_imbalance = imbalance;
                    set.slot_count = slots;
                    dict_create( &d, epochs, &set, &met );

                    pthread_t  *pts  = malloc( threads * sizeof( pthread_t  ));
                    threadargs *args = malloc( threads * sizeof( threadargs ));

                    timespec start, end;
                    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                    for ( int tid = 0; tid < threads; tid++ ) {
                        args[tid].dict = d;
                        args[tid].operations = operations;
                        args[tid].id = tid;
                        pthread_create( &(pts[tid]), NULL, thread_do_inserts, &(args[tid]) );
                    }
                    for ( int tid = 0; tid < threads; tid++ ) {
                        pthread_join( pts[tid], NULL );
                    }
                    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

                    timespec duration = time_diff( start, end );
                    fprintf( stdout, "INSERT, %i, %i, %i, %i, %i, %lli:%li, %zi, %zi\n",
                        epochs,
                        threads,
                        slots,
                        imbalance,
                        operations,
                        (long long)duration.tv_sec,
                        duration.tv_nsec,
                        d->rebalanced,
                        d->epoch_count
                    );
                    fflush( stdout );

                    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                    for ( int tid = 0; tid < threads; tid++ ) {
                        args[tid].dict = d;
                        args[tid].operations = operations;
                        args[tid].id = tid;
                        pthread_create( &(pts[tid]), NULL, thread_do_lookups, &(args[tid]) );
                    }
                    for ( int tid = 0; tid < threads; tid++ ) {
                        pthread_join( pts[tid], NULL );
                    }
                    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

                    duration = time_diff( start, end );
                    fprintf( stdout, "LOOKUP, %i, %i, %i, %i, %i, %lli:%li, %zi, %zi\n",
                        epochs,
                        threads,
                        slots,
                        imbalance,
                        operations,
                        (long long)duration.tv_sec,
                        duration.tv_nsec,
                        d->rebalanced,
                        d->epoch_count
                    );
                    fflush( stdout );

                    free( pts );
                    free( args );
                    dict_free( &d );
                }
            }
        }
    }
}


void *thread_do_inserts( void *ptr ) {
    threadargs *args = ptr;
    dict *d = args->dict;

    for ( uint64_t i = 0; i < args->operations; i++ ) {
        kv *it = new_kv( args->id, i );
        dict_stat s = dict_insert( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }
        kv_ref( d, it, -1 );
    }

    return NULL;
}

void *thread_do_lookups( void *ptr ) {
    threadargs *args = ptr;
    dict *d = args->dict;

    for ( uint64_t i = 0; i < args->operations; i++ ) {
        kv *it = new_kv( args->id, i );
        kv *got = NULL;
        dict_get( d, it, (void **)&got );
        assert( got->value == i );
        assert( got->cat == args->id );
        kv_ref( d, it, -1 );
        kv_ref( d, got, -1 );
    }

    return NULL;
}

kv *new_kv( char cat, uint64_t val ) {
    kv *it = malloc( sizeof( kv ));
    memset( it, 0, sizeof( kv ));
    it->refcount = 1;
    it->fnv_hash = 0;
    it->value = val;
    it->cat = cat;
    return it;
}

kv *rnd_kv( char cat, unsigned int *seedp ) {
    return new_kv(
        cat,
        rand_r( seedp ) % 32000
    );
}

void kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val ) {
    kv *k  = key;
    kv *ov = old_val ? old_val : &NKV;
    kv *nv = new_val ? new_val : &NKV;

    if ( k->cat == 'V' ) return;

    fprintf( stderr, "\33[2K\r[%zi]", k->value );
    fflush( stderr );
}

void kv_ref( dict *d, void *ref, int delta ) {
    if ( ref == NULL ) return;
    if ( ref == &NKV ) return;
    kv *r = ref;
    assert( r->refcount != 0 );
    size_t refs = __sync_add_and_fetch( &(r->refcount), delta );
    if ( refs == 0 ) free( ref );
}

size_t kv_loc( dict_settings *s, void *key ) {
    kv *k = key;
    //return k->value % s->slot_count;

    if ( !k->fnv_hash ) {
        k->fnv_hash = hash_bytes( key, 3 );
    }
    return k->fnv_hash % s->slot_count;
}

int kv_cmp( dict_settings *s, void *key1, void *key2 ) {
    if ( key1 == key2 ) return 0;

    kv *k1 = key1;
    kv *k2 = key2;

    if ( k1->value > k2->value ) return -1;
    if ( k1->value < k2->value ) return  1;

    if ( k1->cat > k2->cat ) return -1;
    if ( k1->cat < k2->cat ) return  1;

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

