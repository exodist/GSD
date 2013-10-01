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

typedef struct object           object;
typedef struct object_component object_component;
typedef struct object_component object_role;
typedef struct object_class     object_class;
typedef struct object_instance  object_instance;
typedef struct object_simple    object_simple;

typedef struct collection collection;
typedef struct collector  collector;
typedef struct region     region;

typedef struct string_snip string_snip;

#define STRING_TYPE_START 240
#define SIMPLE_TYPE_START 10

typedef enum {
    GC_FREE  = 0,
    GC_CLASS = 1,
    GC_ROLE  = 2,
    GC_INST  = 3,

    GC_POINTER = SIMPLE_TYPE_START,
    GC_INT     = SIMPLE_TYPE_START + 1,
    GC_DEC     = SIMPLE_TYPE_START + 2,
    GC_HANDLE  = SIMPLE_TYPE_START + 3,
    GC_DICT    = SIMPLE_TYPE_START + 4,
    GC_BOOL    = SIMPLE_TYPE_START + 5,
    GC_UNDEF   = SIMPLE_TYPE_START + 6,

    GC_SNIP    = STRING_TYPE_START,
    GC_STRING  = STRING_TYPE_START + 1,
    GC_STRINGC = STRING_TYPE_START + 2,
    GC_ROPE    = STRING_TYPE_START + 3,
} primitive;

struct string_snip {
    unsigned int bytes : 4;
    unsigned int chars : 4;
    uint8_t data[7];
};

struct object {
    primitive type : 8;
    enum { GC_UNCHECKED, GC_CHECKING, GC_CHECKED } state : 8;

    uint8_t  permanent : 8;
    uint8_t  epoch     : 8;
    uint32_t ref_count : 32;
};

struct object_component {
    object object;

    dict *roles;
    dict *symbols;
    dict *attributes;

    dict *composed;
};

struct object_class {
    object_component component;

    object_class *parent;

    type_storage storage;

    uint8_t ref_count_only;
};

struct object_instance {
    object object;

    object_component *spec;

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

    object_class types[12];
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
