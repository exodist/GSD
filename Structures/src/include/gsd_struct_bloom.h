#ifndef GSD_STRUCT_BLOOM_H
#define GSD_STRUCT_BLOOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

bloom *bloom_create_k(singularity *s, size_t size, uint8_t k, ...);
bloom *bloom_create(singularity *s, size_t size, uint8_t k, hasher *h);

// Returns true if the new insert collides with an existing one
// -1 is returned on error
int bloom_insert(bloom *b, const object *o);

// Returns true if the item is *probably* present
// -1 is returned on error
int bloom_lookup(bloom *b, const object *o);

void free_bloom(bloom *b);

#endif
