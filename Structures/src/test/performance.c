#include "../include/gsd_dict.h"
#include "../include/gsd_dict_return.h"
#include "structure.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#define new_kv( a ) x_new_kv( a, __FILE__, __LINE__ )

typedef struct timespec timespec;
typedef struct kv kv;
struct kv {
    uint64_t  value;
    size_t    refcount;
    uint64_t  fnv_hash;
    char      *fname;
    size_t    line;
};

kv NKV = { 0, 0, 0, __FILE__, __LINE__ };

// If true FNV will be used to locate and compare keys, otherwise the raw
// integer value is used (which is a worst case scenario as that means sorted
// insert into all the trees.
int USE_FNV = 1;

void   kv_ref( dict *d, void *ref, int delta );
size_t kv_loc( size_t slot_count, void *meta, void *key );
int    kv_cmp( void *meta, void *key1, void *key2 );
void   kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val );

uint64_t hash_bytes( uint8_t *data, size_t length );

kv *x_new_kv( uint64_t val, char *file, size_t line );

void *thread_do_inserts( void *ptr );
void *thread_do_lookups( void *ptr );
void *thread_do_updates( void *ptr );

typedef struct threadargs threadargs;
struct threadargs {
    dict *dict;
    int  operations;
    int  id;
    int  threads;
};

timespec time_diff(timespec start, timespec end) {
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
    size_t min_ops = 1048576 / 2;
    size_t max_ops = 1048576;
    size_t min_slots = 1024;
    size_t max_slots = 1024 * 4 * 4;
    size_t min_imbalance = 8;
    size_t max_imbalance = 16;
    size_t min_threads = 1;
    size_t max_threads = 8;
    size_t min_epochs = 4;
    size_t max_epochs = 4;

    dict_settings set = { 0, 0, 1, NULL };
    dict_methods  met = { kv_cmp, kv_loc, kv_change, kv_ref };
    dict *d = NULL;

    fprintf( stdout,
        "Max Epochs, "
        "Threads, "
        "Max Imbalance, "
        "Slot Count, "
        "Operations, "
        "Insert Time, "
        "Rebalance Time, "
        "Lookup Time, "
        "Update Time, "
        "Resize Time, "
        "Rebalanced, "
        "Used Epochs\n"
    );
    fflush( stdout );

    for ( size_t epochs = min_epochs; epochs <= max_epochs; epochs *= 2 ) {
        for ( size_t threads = min_threads; threads <= max_threads; threads *= 2 ) {
            for ( size_t imbalance = min_imbalance; imbalance <= max_imbalance; imbalance *= 2 ) {
                for ( size_t slot_count = min_slots; slot_count <= max_slots; slot_count *= 4 ) {
                    for ( size_t operations = min_ops; operations <= max_ops; operations *= 2 ) {
                        fprintf( stdout, "%zi, %zi, %zi, %zi, %zi, ",
                            epochs,
                            threads,
                            imbalance,
                            slot_count,
                            operations
                        );
                        fflush( stdout );

                        set.max_imbalance = imbalance;
                        set.max_internal_threads = threads;
                        set.slot_count = slot_count;
                        dict_create( &d, epochs, set, met );

                        pthread_t  *pts  = malloc( threads * sizeof( pthread_t  ));
                        threadargs *args = malloc( threads * sizeof( threadargs ));

                        timespec start, end;
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                        for ( int tid = 0; tid < threads; tid++ ) {
                            args[tid].dict = d;
                            args[tid].operations = operations / threads;
                            args[tid].id = tid;
                            args[tid].threads = threads;
                            pthread_create( &(pts[tid]), NULL, thread_do_inserts, &(args[tid]) );
                        }
                        for ( int tid = 0; tid < threads; tid++ ) {
                            pthread_join( pts[tid], NULL );
                        }
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
                        timespec insert_duration = time_diff( start, end );
                        fprintf( stdout, "%lli.%09li, ",
                            (long long)insert_duration.tv_sec, insert_duration.tv_nsec
                        );
                        fflush( stdout );

                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                        dict_rebalance( d, 3, threads );
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
                        timespec rebalance_duration = time_diff( start, end );
                        fprintf( stdout, "%lli.%09li, ",
                            (long long)rebalance_duration.tv_sec, rebalance_duration.tv_nsec
                        );
                        fflush( stdout );

                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                        for ( int tid = 0; tid < threads; tid++ ) {
                            args[tid].dict = d;
                            args[tid].operations = operations / threads;
                            args[tid].id = tid;
                            args[tid].threads = threads;
                            pthread_create( &(pts[tid]), NULL, thread_do_lookups, &(args[tid]) );
                        }
                        for ( int tid = 0; tid < threads; tid++ ) {
                            pthread_join( pts[tid], NULL );
                        }
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
                        timespec lookup_duration = time_diff( start, end );
                        fprintf( stdout, "%lli.%09li, ",
                            (long long)lookup_duration.tv_sec, lookup_duration.tv_nsec
                        );
                        fflush( stdout );

                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                        for ( int tid = 0; tid < threads; tid++ ) {
                            args[tid].dict = d;
                            args[tid].operations = operations / threads;
                            args[tid].id = tid;
                            args[tid].threads = threads;
                            pthread_create( &(pts[tid]), NULL, thread_do_updates, &(args[tid]) );
                        }
                        for ( int tid = 0; tid < threads; tid++ ) {
                            pthread_join( pts[tid], NULL );
                        }
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
                        timespec update_duration = time_diff( start, end );
                        fprintf( stdout, "%lli.%09li, ",
                            (long long)update_duration.tv_sec, update_duration.tv_nsec
                        );
                        fflush( stdout );

                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
                        set.slot_count = slot_count * 2;
                        rstat conf = dict_reconfigure( d, set, threads );
                        if ( conf.bit.error ) {
                            fprintf( stderr, "\nERROR: %i, %s\n", conf.bit.error, dict_stat_message(conf));
                        }
                        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
                        timespec resize_duration = time_diff( start, end );
                        fprintf( stdout, "%lli.%09li, ",
                            (long long)resize_duration.tv_sec, resize_duration.tv_nsec
                        );
                        fflush( stdout );

                        fprintf( stdout, "%zi, %i\n",
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
}


void *thread_do_inserts( void *ptr ) {
    threadargs *args = ptr;
    dict *d = args->dict;

    uint64_t parts = args->operations / args->threads;
    uint64_t start = args->id * parts;
    uint64_t end = start + parts;

    for ( uint64_t i = start; i < end; i++ ) {
        kv *it = new_kv( i );
        dict_stat s = dict_insert( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }
        kv_ref( d, it, -1 );
    }

    return NULL;
}

void *thread_do_updates( void *ptr ) {
    threadargs *args = ptr;
    dict *d = args->dict;

    uint64_t parts = args->operations / args->threads;
    uint64_t start = args->id * parts;
    uint64_t end = start + parts;

    for ( uint64_t i = start; i < end; i++ ) {
        kv *it = new_kv( i );
        kv *va = new_kv( i + 1 );
        dict_stat s = dict_update( d, it, va );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }
        kv_ref( d, it, -1 );
        kv_ref( d, va, -1 );
    }

    return NULL;
}

void *thread_do_lookups( void *ptr ) {
    threadargs *args = ptr;
    dict *d = args->dict;

    uint64_t parts = args->operations / args->threads;
    uint64_t start = args->id * parts;
    uint64_t end = start + parts;

    for ( uint64_t i = start; i < end; i++ ) {
        kv *it = new_kv( i );
        kv *got = NULL;
        dict_get( d, it, (void **)&got );
        assert( got && got->value == i );
        if( !got || got->value != i ) {
            fprintf( stderr, "Lost: %zu\n", i );
        }
        kv_ref( d, it, -1 );
        kv_ref( d, got, -1 );
    }

    return NULL;
}

kv *x_new_kv( uint64_t val, char *file, size_t line ) {
    kv *it = malloc( sizeof( kv ));
    memset( it, 0, sizeof( kv ));
    it->refcount = 1;
    it->value = val;
    it->fname = file;
    it->line = line;
    return it;
}

void kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val ) {
    return;
    kv *k  = key;

    fprintf( stderr, "\33[2K\r[%7zi]", k->value );
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


