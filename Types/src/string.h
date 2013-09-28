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

struct string_header {
    uint32_t bytes;
    uint32_t chars;
    uint64_t hash;
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

    size_t   child_count;
    object **children;
};

struct string_iterator {
    object_simple *item;
    size_t index;
    string_iterator *stack;

    enum { I_ANY = 0, I_BYTES, I_CHARS } units : 8;
    uint8_t complete;
};

const uint8_t *iterator_next_part( string_iterator **ip, ucs4_t *c, int *s );

void free_string( object *o );

#endif
