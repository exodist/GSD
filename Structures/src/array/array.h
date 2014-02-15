#ifndef ARRAY_ARRAY_H
#define ARRAY_ARRAY_H

#include "../include/gsd_struct_array.h"
#include "../include/gsd_struct_bitmap.h"
#include "../common/sref.h"
#include "../../../PRM/src/include/gsd_prm.h"

typedef struct array_data   array_data;
typedef struct array_bounds array_bounds;

struct array_data {
    size_t size;
    usref *items;
};

struct array_bounds {
    int64_t offset;
    int64_t count;
    int64_t inner_offset;
    uint8_t offset_lock;
    uint8_t count_lock;
};

struct array {
    prm *p;
    refdelta *delta;
    size_t grow;

    array_data *inner;
    array_data *outer;

    array_bounds *bounds;
};

usref *array_usref(array *a, int64_t idx, int vivify);
usref *array_push(array *a);
usref *array_pop(array *a);
usref *array_shift(array *a);
usref *array_unshift(array *a);

int array_grow(array *a, int side);

#endif
