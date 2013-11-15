#include <string.h>
#include "stringdict.h"
#include "../../Hashlib/src/include/gsd_hashlib.h"

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

    r->type = STRD_REF;
    r->val = p;

    return r;
}

get_stat     strd_get( dict *d, uint8_t *key            );
dict_stat    strd_set( dict *d, uint8_t *key, void *val );
dict_stat strd_update( dict *d, uint8_t *key, void *val );
dict_stat strd_insert( dict *d, uint8_t *key, void *val );
dict_stat strd_delete( dict *d, uint8_t *key            );

void string_dict_ref( dict *d, void *ref, int delta ) {
    strd_ref *r = ref;
    r->refs += delta;
    if (r->refs != 0) return;

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
    if (!k->hash_set) {
        k->hash = fnv_hash(k->ref.val, strlen(k->ref.val), NULL);
        k->hash_set = 1;
    }

    return k->hash;
}

