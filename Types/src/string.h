#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stdlib.h>
#include "include/type_api.h"

typedef struct object object;

typedef struct string_header   string_header;
typedef struct string          string;
typedef struct string_rope     string_rope;
typedef struct string_snip     string_snip;
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

struct string_rope {
    string_header head;

    size_t  child_count;
    object *children;
};

struct string_iterator {
    object *item;
    size_t index;
    string_iterator *stack;

    enum { I_ANY = 0, I_BYTES, I_CHARS } units : 8;
};

struct string_snip {
    unsigned int bytes : 4;
    unsigned int chars : 4;
    uint8_t data[7];
};

#endif
