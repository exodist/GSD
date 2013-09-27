#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdint.h>
#include "structures.h"
#include "GSD_Dictionary/src/include/gsd_dict.h"

uint64_t hash_object       ( object *o                                  );
uint64_t hash_bytes        ( uint8_t *data, size_t length               );
uint64_t hash_append_bytes ( uint8_t *data, size_t length, uint64_t key );

#endif
