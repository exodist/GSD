#include <string.h>
#include "stringdict.h"
#include "../../Hashlib/src/include/gsd_hashlib.h"
#include "error.h"

static dict_methods METHODS = {
    .cmp    = string_dict_cmp,
    .loc    = string_dict_loc,
    .ref    = string_dict_ref,
    .change = NULL,
};

dict *new_string_dict(size_t slots, void (release)(void *)) {
    return dict_build( slots, METHODS, release );
}

strd_key *str_to_key( uint8_t *s ) {
    size_t len = strlen( s );

    strd_key *k = malloc( sizeof( strd_key ));
    if (!k) return NULL;
    memset( k, 0, sizeof( strd_key ));
    k->ref.type = STRD_KEY;

    k->ref.refs = 1;

    k->ref.val = malloc( len );
    if ( !k->ref.val ) {
        free(k);
        return NULL;
    }

    memcpy( k->ref.val, s, len );

    return k;
}

strd_ref *ptr_to_ref( void *p ) {
    strd_ref *r = malloc( sizeof( strd_ref ));
    if (!r) return NULL;

    memset( r, 0, sizeof( strd_ref ));

    r->refs = 1;
    r->type = STRD_REF;
    r->val = p;

    return r;
}

get_stat strd_get( dict *d, uint8_t *key ) {
    get_stat out = {
        .got  = NULL,
        .stat = { .num = 0 }
    };

    strd_key *k = str_to_key( key );
    if (!k) {
        out.stat = rstat_mem;
        return out;
    }

    out.stat = dict_get( d, k, &(out.got));

    string_dict_ref( d, k, -1 );

    return out;
}

dict_stat strd_set( dict *d, uint8_t *key, void *val ) {
    strd_key *k = str_to_key( key );
    if (!k) return rstat_mem;

    dict_stat out = dict_set( d, k, val );
    string_dict_ref( d, k, -1 );
    return out;
}

dict_stat strd_update( dict *d, uint8_t *key, void *val ) {
    strd_key *k = str_to_key( key );
    if (!k) return rstat_mem;

    dict_stat out = dict_update( d, k, val );
    string_dict_ref( d, k, -1 );
    return out;
}

dict_stat strd_insert( dict *d, uint8_t *key, void *val ) {
    strd_key *k = str_to_key( key );
    if (!k) return rstat_mem;

    dict_stat out = dict_insert( d, k, val );
    string_dict_ref( d, k, -1 );
    return out;
}

dict_stat strd_delete( dict *d, uint8_t *key ) {
    strd_key *k = str_to_key( key );
    if (!k) return rstat_mem;

    dict_stat out = dict_delete( d, k );
    string_dict_ref( d, k, -1 );
    return out;
}

void string_dict_ref( dict *d, void *ref, int delta ) {
    strd_ref *r = ref;
    size_t refs = __atomic_add_fetch( &(r->refs), delta, __ATOMIC_ACQ_REL );
    if (refs) return;

    dict_settings s = dict_get_settings( d );

    if (r->type == STRD_KEY) {
        free( r->val );
    }
    else if (s.meta) {
        void (*release)(void *) = s.meta;
        release( r->val );
    }

    free(r);
}

int string_dict_cmp( void *meta, void *key1, void *key2, uint8_t *error ) {
    strd_key *k1 = key1;
    strd_key *k2 = key2;

    uint64_t h1 = strd_hash( k1 );
    uint64_t h2 = strd_hash( k2 );

    if (h1 > h2) return  1;
    if (h1 < h2) return -1;

    size_t idx = 0;
    while(1) {
        int diff = k1->ref.val[idx] - k2->ref.val[idx];

        // Keys are identical up until the null char.
        if (!diff && k1->ref.val[idx] == '\0') break;

        idx++;

        // No difference, go to next char
        if (!diff) continue;

        if (diff > 0) return  1;
                      return -1;
    }

    return 0;
}

size_t string_dict_loc( size_t slot_count, void *meta, void *key, uint8_t *error ) {
    strd_key *k = key;
    uint64_t h = strd_hash( k );
    return h % slot_count;
}

uint64_t strd_hash( strd_key *k ) {
    while(!__atomic_load_n(&(k->hash_set), __ATOMIC_CONSUME)) {
        uint8_t expect = 0;
        int success = __atomic_compare_exchange_n(
            &(k->hash_set),
            &expect,
            1,
            0,
            __ATOMIC_RELEASE,
            __ATOMIC_RELAXED
        );
        if (!success) continue;

        __atomic_store_n(
            &(k->hash),
            fnv_hash(k->ref.val, strlen(k->ref.val), NULL),
            __ATOMIC_RELEASE
        );

        __atomic_store_n(
            &(k->hash_set),
            1,
            __ATOMIC_RELEASE
        );
    }

    return k->hash;
}

