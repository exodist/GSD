#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdint.h>
#include "structures.h"
#include "include/exceptions_api.h"
#include "GSD_Dictionary/src/include/gsd_dict.h"

#define FNV_SEED  14695981039346656037UL
#define FNV_PRIME 1099511628211

typedef uint64_t (hash_function)(uint8_t *data, size_t  length, uint64_t current);
typedef struct gc_dict_meta gc_dict_meta;

extern dict_methods DOBJ_METH;

struct gc_dict_meta {
    uint64_t       initial_state;
    hash_function *hash_function;

    object *upper_int_bound;
    object *lower_int_bound;
};

uint64_t hash_object ( gc_dict_meta *m, object *o, exception *e );

uint64_t fnv_hash_bytes ( uint8_t *data, size_t length, uint64_t key );

int    obj_cmp( void *meta, void *key1, void *key2, uint8_t *e );
size_t obj_loc( size_t slot_count, void *meta, void *key, uint8_t *e );

#endif
