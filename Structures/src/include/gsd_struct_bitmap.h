#ifndef GSD_STRUCT_BITMAP_H
#define GSD_STRUCT_BITMAP_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"

bitmap *bitmap_create(int64_t bits);
bitmap *bitmap_clone(bitmap *b, int64_t bits);

int bitmap_get(bitmap *b, int64_t idx);
int bitmap_set(bitmap *b, int64_t idx, int state);

int64_t bitmap_fetch(bitmap *b, int state, int64_t max);

void bitmap_reset(bitmap *b, unsigned int state);

void bitmap_free(bitmap *b);

#endif
