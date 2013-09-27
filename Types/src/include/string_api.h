#ifndef GSD_GC_STRING_API_H
#define GSD_GC_STRING_API_H

#include <stdio.h>
#include <stdint.h>

typedef struct object object;

typedef struct string_iterator string_iterator;

size_t string_bytes( object *o );
size_t string_chars( object *o );

string_iterator iterate_string( object *s );

uint8_t  iterator_next_utf8( string_iterator *i );
uint32_t iterator_next_unic( string_iterator *i );
uint8_t  iterator_next_byte( string_iterator *i );

// Byte based comparison.
int string_compare( object *a, object *b );

#endif
