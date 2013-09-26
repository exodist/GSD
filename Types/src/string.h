#ifndef GC_STRING_H
#define GC_STRING_H

#include <stdint.h>
#include <stdlib.h>

typedef struct object object;

typedef struct string_header   string_header;
typedef struct string          string;
typedef struct string_rope     string_rope;
typedef struct string_rope     string_snip;
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

    string_iterator *stack;
    size_t index;
};

struct string_snip {
    unsigned int bytes : 4;
    unsigned int chars : 4;
    uint8_t data[7];
};

string_iterator iterate_string( object *s );

uint8_t *iterator_next_utf8( string_iterator *i );
uint32_t iterator_next_char( string_iterator *i );
uint8_t  iterator_next_byte( string_iterator *i );

int string_compare( object *a, object *b );

#endif
