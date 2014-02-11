#ifndef BITMAP_BITMAP_H
#define BITMAP_BITMAP_H

#include "../include/gsd_struct_bitmap.h"

#define FULL64 0xFFFFFFFFFFFFFFFFUL
#define FULL32 0xFFFFFFFFU
#define FULL16 0xFFFFU
#define FULL08 0xFFU

struct bitmap {
    int64_t bits;
    int64_t bytes;
    uint64_t *data;
};

bitmap *bitmap_alloc(int64_t bits);

#endif
