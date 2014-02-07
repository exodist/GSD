#ifndef GSD_STRUCT_SBLOOM_H
#define GSD_STRUCT_SBLOOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

sbloom *sbloom_create_k(void *meta, size_t size, uint8_t k, ...);

sbloom *sbloom_create(void *meta, size_t size, uint8_t k, hasher *bh);

// Returns true if the new insert collides with an existing one
// -1 is returned on error
int sbloom_insert(sbloom *b, const char *item);

// Returns true if the item is *probably* present
// -1 is returned on error
int sbloom_lookup(sbloom *b, const char *item);

void free_sbloom(sbloom *b);

#endif
