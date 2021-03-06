#ifndef STRINGDICT_H
#define STRINGDICT_H

#include "include/gsd_string_dict.h"

typedef struct strd_ref strd_ref;
typedef struct strd_key strd_key;

struct strd_ref {
    enum { STRD_REF = 1, STRD_KEY = 2 } type;

    size_t refs;
    void  *val;
};

struct strd_key {
    strd_ref ref;
    uint64_t hash;
    uint8_t  hash_set;
};

void   string_dict_ref( dict *d, void *ref, int delta );
int    string_dict_cmp( void *meta, void *key1, void *key2, uint8_t *error );
size_t string_dict_loc( size_t slot_count, void *meta, void *key, uint8_t *error );

uint64_t strd_hash( strd_key *k );

strd_key *str_to_key( char *s );
strd_ref *ptr_to_ref( void *p );

#endif
