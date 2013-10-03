#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "GSD_Dictionary/src/include/gsd_dict.h"
#include "include/object_api.h"
#include "include/type_api.h"
#include "include/exceptions_api.h"
#include "include/constructor_api.h"
#include "string.h"

typedef struct object      object;
typedef struct component   component;
typedef struct component   role;
typedef struct type        type;
typedef struct instance    instance;
typedef struct accessor    accessor;
typedef struct attribute   attribute;

typedef struct string_header   string_header;
typedef struct string          string;
typedef struct string_rope     string_rope;
typedef struct string_const    string_const;
typedef struct string_snip     string_snip;

typedef struct string_iterator string_iterator;
typedef struct string_iterator_stack string_iterator_stack;

#define STRING_TYPE_START 240

typedef enum {
    GC_INST      = 0,
    GC_TYPE      = 1,
    GC_ROLE      = 2,
    GC_POINTER   = 3,
    GC_INT       = 4,
    GC_DEC       = 5,
    GC_HANDLE    = 6,
    GC_DICT      = 7,
    GC_BOOL      = 8,
    GC_UNDEF     = 9,
    GC_ACCESSOR  = 10,
    GC_ATTRIBUTE = 11,
    GC_CFUNCTION = 12,
    GC_CMETHOD   = 13,

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

struct component {
    dict *roles;
    dict *symbols;
    dict *attributes;
    dict *composed;
};

struct type {
    component    component;
    type        *parent;
    type_storage storage;
    uint8_t      ref_count_only;
};

struct accessor {
    size_t index;
    access_type access;

    attribute *attribute;
};

struct attribute {
    dict *delegates;

    object *reader;
    object *writer;
    object *clear;
    object *check;

    object *builder;
    object *transmute;

    component *restrictions;
    uint16_t   restriction_count;

    uint8_t required;
};

struct string_header {
    uint64_t bytes : 64;
    uint64_t chars : 63;

    unsigned int hash_set : 1;
    uint64_t     hash;
};

struct string {
    string_header head;

    uint8_t first_byte;
};

struct string_const {
    string_header head;

    const uint8_t *string;
};

struct string_rope {
    string_header head;

    uint32_t depth;
    uint32_t child_count;
    object **children;
};

struct string_iterator_stack {
    object *item;
    size_t  index;
};

struct string_iterator {
    enum { I_ANY = 0, I_BYTES, I_CHARS } units : 16;
    unsigned int complete : 16;
    uint32_t stack_index  : 32;
    string_iterator_stack stack;
};

struct object {
    primitive primitive : 8;
    enum { GC_UNCHECKED, GC_CHECKING, GC_CHECKED } state : 8;

    uint8_t  permanent : 8;
    uint8_t  epoch     : 8;
    uint32_t ref_count : 32;

    union {
        // Truly Primitive
        int64_t     integer;
        double      decimal;

        // String Primitives
        string_header   *string_header;
        string          *string;
        string_rope     *string_rope;
        string_const    *string_const;
        string_snip      string_snip;
        string_iterator *string_iterator;

        // Object composition primitives
        component  *component;
        role       *role;
        type       *type;
        accessor   *accessor;
        attribute  *attribute;

        // External primitives
        void       *ptr;
        FILE       *handle;
        dict       *dict;
        object     *attrs;
        cfunction  *cfunction;
        cmethod    *cmethod;
    } data;
};

struct instance {
    object     object;
    component *spec;
};

#endif
