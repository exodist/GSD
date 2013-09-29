#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stdlib.h>
#include "include/type_api.h"
#include "structures.h"
#include <stdlib.h>
#include <unistr.h>

typedef struct object        object;
typedef struct object_simple object_simple;

typedef struct string_header   string_header;
typedef struct string          string;
typedef struct string_rope     string_rope;
typedef struct string_const    string_const;

typedef struct string_iterator string_iterator;
typedef struct string_iterator_stack string_iterator_stack;

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
    object_simple **children;
};

struct string_iterator_stack {
    object_simple *item;
    size_t         index;
};

struct string_iterator {
    enum { I_ANY = 0, I_BYTES, I_CHARS } units : 16;
    unsigned int complete : 16;
    uint32_t stack_index  : 32;
    string_iterator_stack stack;
};

const uint8_t *iterator_next_part( string_iterator *i, ucs4_t *c );

void free_string( object *o );

#endif
