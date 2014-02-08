#ifndef BITMAP_H
#define BITMAP_H

#include "../include/gsd_struct_bitmap.h"

struct bitmap {
    int64_t bits;
    int64_t bytes;
    uint64_t *data;
};

bitmap *bitmap_alloc(int64_t bits);

#endif
