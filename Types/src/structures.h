#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "GSD_Dictionary/src/include/gsd_dict.h"
#include "include/object_api.h"
#include "include/collector_api.h"
#include "include/type_api.h"
#include "include/exceptions_api.h"
#include "string.h"

typedef struct object        object;
typedef struct object_type   object_type;
typedef struct object_typed  object_typed;
typedef struct object_simple object_simple;

typedef struct collection collection;
typedef struct collector  collector;
typedef struct region     region;

struct object {
    primitive type : 8;
    enum { GC_UNCHECKED, GC_CHECKING, GC_CHECKED } state : 8;

    uint8_t  permanent : 8;
    uint8_t  epoch     : 8;
    uint32_t ref_count : 32;
};

struct object_type {
    object object;

    object_type *parent;

    dict *roles;
    dict *symbols;
    dict *attributes;

    type_storage storage;

    uint8_t immutable;
    uint8_t ref_count_only;
};

struct object_typed {
    object object;

    object_type *type;

    union {
        void *ptr;
        dict *dict;
    } storage;
};

struct object_simple {
    object object;

    union {
        void       *ptr;
        int64_t     integer;
        double      decimal;
        string_snip snip;
        FILE       *handle;
        dict       *dict;
    } simple_data;
};

struct collector {
    dict *roots;

    collection *associated;
    collection *available;

    pthread_t *thread;

    object_type types[12];
};

struct region {
    size_t units;
    size_t count;
    size_t index;
    void  *start;

    region *next;
};

struct collection {
    region *simple;
    region *typed;
    region *types;

    object *free_simple;
    object *free_typed;
    object *free_types;

    collection *next;
};

#endif
