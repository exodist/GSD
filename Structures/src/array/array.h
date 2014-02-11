#ifndef ARRAY_ARRAY_H
#define ARRAY_ARRAY_H

#include "../include/gsd_struct_array.h"
#include "../include/gsd_struct_bitmap.h"
#include "../common/sref.h"
#include "../../../PRM/src/include/gsd_prm.h"

#define LOOKUP_OFFSET   1
#define LOOKUP_FREE     0
#define LOOKUP_BLOCKED  0xFFFFFFFF
#define STORAGE_FREE    0UL
#define STORAGE_BLOCKED 0xFFFFFFFFFFFFFFFFUL

typedef struct array_data array_data;

struct array_data {
    size_t size;

    union {
        uint32_t parts[2];
        uint64_t both;
    } ends; 

    uint32_t *lookup;
    sref    **storage;
    bitmap   *available;
};

struct array {
    prm *p;
    refdelta *delta;

    array_data *current;
    array_data *resize;
};

#endif
