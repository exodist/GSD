#include "../include/gsd_dict.h"
#include "../include/gsd_dict_return.h"
#include "../balance.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Specification:

Keys and Values should be ref counted
Should have a change hook (replace line with "key: old -> new")
Should use fnv_hash for hash-table
Should use fnv_hash for compare

 == Should have these threads ==
 * Dot dumper, dump every 2 seconds.
 -- These need unique keys --
 * Add many unique keys, then delete them, do it again (assert value each time)
 * Add many unique keys, then deref them, do it again (assert value each time)
 -- None of these use unique keys --
 * insert random keys (in a range) (probably a list, go to random item in list,
   remove it from list and insert. Complete when list is empty. Track
   success/fail
 * Constantly update random keys from range track success/fail
 * Constantly set random keys from range
 * Constantly delete keys
 * Constantly deref keys
 * Constantly make references
 * Constantly iterate all nodes... count them?
 * cmp_* transactional operations (track success/fail)
\*/

typedef struct kv kv;
struct kv {
    uint16_t value;
    char     cat;
    size_t   refcount;
    uint64_t fnv_hash;
};

kv NKV = { 1, 'N', 0, 0 };

uint64_t hash_bytes( uint8_t *data, size_t length );

void   kv_change( dict *d, void *meta, void *key, void *old_val, void *new_val );
void   kv_ref( dict *d, void *meta, void *ref, int delta );
size_t kv_loc( dict_settings *s, void *key );
int    kv_cmp( dict_settings *s, void *key1, void *key2 );
char  *kv_dot( void *key, void *val );
int    kv_handler( void *key, void *value, void *args );

kv *new_kv( char cat, uint16_t val );

void *thread_dot_dumper( void *ptr );
void *thread_uniq_delete( void *ptr );
void *thread_uniq_deref( void *ptr );
void *thread_rand_insert( void *ptr );
void *thread_rand_update( void *ptr );
void *thread_rand_set( void *ptr );
void *thread_rand_get( void *ptr );
void *thread_rand_delete( void *ptr );
void *thread_rand_deref( void *ptr );
void *thread_rand_ref( void *ptr );
void *thread_iterate_all( void *ptr );
void *thread_transactions( void *ptr );
void *thread_rand( void *ptr );

// How long to run in seconds
int LENGTH = 20;
int START = 0;

int main() {
    dict_settings set = { 16, 2, NULL };
    dict_methods  met = { kv_cmp, kv_loc, kv_change, kv_ref };
    dict *d = NULL;
    dict_create( &d, 12, &set, &met );

    START = time(NULL);

    pthread_t threads[15];
    pthread_create( &(threads[0]), NULL, thread_uniq_delete, d );
    pthread_create( &(threads[1]), NULL, thread_uniq_deref, d );
    pthread_create( &(threads[2]), NULL, thread_dot_dumper, d );
    pthread_create( &(threads[3]), NULL, thread_rand, d );
    pthread_create( &(threads[4]), NULL, thread_iterate_all, d );

    pthread_create( &(threads[5]), NULL, thread_rand_insert, d );
    pthread_create( &(threads[6]), NULL, thread_rand_update, d );
    pthread_create( &(threads[7]), NULL, thread_rand_set, d );
    pthread_create( &(threads[8]), NULL, thread_rand_get, d );
    pthread_create( &(threads[9]), NULL, thread_rand_delete, d );
    pthread_create( &(threads[10]), NULL, thread_rand_deref, d );
    //pthread_create( &(threads[11]), NULL, thread_rand_ref, d );


    pthread_join( threads[0], NULL );
    pthread_join( threads[1], NULL );
    pthread_join( threads[2], NULL );
    pthread_join( threads[3], NULL );
    pthread_join( threads[4], NULL );

    pthread_join( threads[5], NULL );
    pthread_join( threads[6], NULL );
    pthread_join( threads[7], NULL );
    pthread_join( threads[8], NULL );
    pthread_join( threads[9], NULL );
    pthread_join( threads[10], NULL );
    //pthread_join( threads[11], NULL );

    dict_free( &d );
    printf( "\n" );
}

kv *new_kv( char cat, uint16_t val ) {
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

    printf( "\33[2K\r" );
    printf(
        "%c[%i]:%zi = %c[%i]:%zi -> %c[%i]:%zi",
        k->cat,   k->value,  k->refcount,
        ov->cat, ov->value, ov->refcount,
        nv->cat, nv->value, nv->refcount
    );
    fflush( stdout );
}

void kv_ref( dict *d, void *meta, void *ref, int delta ) {
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

char *kv_dot( void *key, void *val ) {
    assert ( key != RBLD );
    assert ( val != RBLD );

    char *out = malloc( 100 );
    kv *k = key ? key : &NKV;
    kv *v = val ? val : &NKV;

    snprintf(
        out,
        14,
        "%c[%i]:%c[%i]",
        k->cat, k->value,
        v->cat, v->value
    );

    return out;
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

void *thread_uniq_delete( void *ptr ) {
    dict *d = ptr;
    char id = 'A';

    while ( time(NULL) < START + LENGTH ) {
        for ( uint16_t i = 0; i < 1000; i++ ) {
            kv *it = new_kv( id, i );
            dict_stat s = dict_insert( d, it, it );
            if ( s.bit.error ) {
                fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            }
            kv *got = NULL;
            dict_get( d, it, &got );
            assert( got == it );
            kv_ref( d, NULL, it, -2 );
        }
        for ( uint16_t i = 0; i < 1000; i++ ) {
            kv *it = new_kv( id, i );

            dict_stat s = dict_delete( d, it );

            if ( s.bit.error ) {
                fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            }
            assert( it->refcount > 0 );
            kv_ref( d, NULL, it, -1 );
        }
    }

    printf( "\nEND uniq delete\n" );
    return NULL;
}

void *thread_uniq_deref( void *ptr ) {
    dict *d = ptr;
    char id = 'B';

    while ( time(NULL) < START + LENGTH ) {
        for ( uint16_t i = 0; i < 1000; i++ ) {
            kv *it = new_kv( id, i );
            dict_stat s = dict_insert( d, it, it );
            if ( s.bit.error ) {
                fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            }
            kv *got = NULL;
            dict_get( d, it, &got );
            assert( got == it );
            kv_ref( d, NULL, it, -2 );
        }
        for ( uint16_t i = 0; i < 1000; i++ ) {
            kv *it = new_kv( id, i );
            dict_stat s = dict_dereference( d, it );
            if ( s.bit.error ) {
                fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
            }
            kv_ref( d, NULL, it, -1 );
        }
    }

    printf( "\nEND uniq deref\n" );
    return NULL;
}

void *thread_dot_dumper( void *ptr ) {
    dict *d = ptr;

    char *fname = malloc( 200 );

    int i = 1;
    while ( time(NULL) < START + LENGTH + 30) {
        char *dot = dict_dump_dot( d, kv_dot );

        if ( dot != NULL ) {
            snprintf( fname, 200, "/tmp/stressdot-%i.dot", i++ );
            FILE *fp = fopen( fname, "w" );
            fprintf( fp, "%s\n", dot );
            printf( "\nWrote: \"%s\"\n", fname );
            fclose( fp );
        }

        sleep( 5 );
    }

    free( fname );

    printf( "\nEND dot dumper\n" );
    return NULL;
}

void *thread_rand( void *ptr ) {
    dict *d = ptr;
    char id = 'R';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it_k = rnd_kv( id, &seed );
        kv *it_v = rnd_kv( id, &seed );
        dict_stat s = dict_set( d, it_k, it_v );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        kv_ref( d, NULL, it_k, -1 );
        kv_ref( d, NULL, it_v, -1 );
    }

    printf( "\nEND rand\n" );
    return NULL;
}

int kv_handler( void *key, void *value, void *args ) {
    kv *k = key;
    kv *v = value;

    assert( key != NULL );
    assert( key != RBLD );

    assert( k->refcount );

    if ( value != NULL ) {
        assert( value != RBLD );
        assert( v->refcount );
    }

    if ( k->cat == 'V' && value ) {
        assert( k->value == v->value );
    }

    return 0;
}

void *thread_iterate_all( void *ptr ) {
    dict *d = ptr;
    while ( time(NULL) < START + LENGTH ) {
        dict_iterate( d, kv_handler, NULL );
    }
    printf( "\nEND iterate all\n" );
    return NULL;
}

void *thread_rand_insert( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        dict_stat s = dict_insert( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        if ( it->refcount == 0 ) {
            printf( "\n\n\n******\nRET: F:%i, R:%i\n*******\n\n\n", s.bit.fail, s.bit.rebal );
            assert( it->refcount );
        }
        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand insert\n" );
    return NULL;
}

void *thread_rand_update( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        dict_stat s = dict_update( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand update\n" );
    return NULL;
}

void *thread_rand_set( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        dict_stat s = dict_set( d, it, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand set\n" );
    return NULL;
}

void *thread_rand_get( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        kv *got = NULL;
        dict_stat s = dict_get( d, it, &got );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        assert( got != RBLD );
        if ( got != NULL ) {
            assert( it->value == got->value );
            assert( got->refcount != 0 );
            kv_ref( d, NULL, got, -1 );
        }
        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand get\n" );
    return NULL;

}

void *thread_rand_delete( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        dict_stat s = dict_delete( d, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand delete\n" );
    return NULL;
}

void *thread_rand_deref( void *ptr ) {
    dict *d = ptr;
    char id = 'V';
    unsigned int seed = time(NULL);

    while ( time(NULL) < START + LENGTH ) {
        kv *it = rnd_kv( id, &seed );
        dict_stat s = dict_dereference( d, it );
        if ( s.bit.error ) {
            fprintf( stderr, "\nERROR: %i, %s\n", s.bit.error, dict_stat_message(s));
        }

        kv_ref( d, NULL, it, -1 );
    }

    printf( "\nEND rand deref\n" );
    return NULL;
}

void *thread_rand_ref( void *ptr ) {
    return NULL;
}


