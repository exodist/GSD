#ifndef GSD_STRUCT_BLOOM_H
#define GSD_STRUCT_BLOOM_H

#include <stdlib.h>
#include <stdint.h>
#include "gsd_struct_types.h"

typedef struct bloom bloom;

typedef uint64_t(bloom_hasher)(const void *item, void *meta);

bloom *bloom_create(size_t size, uint8_t k, bloom_hasher *bh, void *meta);

// Returns true if the new insert collides with an existing one
// -1 is returned on error
int bloom_insert(bloom *b, const void *item);

// Returns true if the item is *probably* present
// -1 is returned on error
int bloom_lookup(bloom *b, const void *item);

void free_bloom(bloom *b);

#endif
