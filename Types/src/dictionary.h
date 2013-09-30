#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdint.h>
#include "structures.h"
#include "include/exceptions_api.h"
#include "GSD_Dictionary/src/include/gsd_dict.h"

extern dict_methods DOBJ_METH;

uint64_t hash_object       ( object *o,     exception *e                );
uint64_t hash_bytes        ( uint8_t *data, size_t length               );
uint64_t hash_append_bytes ( uint8_t *data, size_t length, uint64_t key );

int    obj_cmp( void *meta, void *key1, void *key2 );
size_t obj_loc( size_t slot_count, void *meta, void *key );

#endif
