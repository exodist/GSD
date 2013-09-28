#ifndef GSD_GC_STRING_API_H
#define GSD_GC_STRING_API_H

#include <stdio.h>
#include <stdint.h>
#include <unitypes.h>

typedef struct object          object;
typedef struct string_iterator string_iterator;

// Get information about the string.
size_t string_bytes( object *o );
size_t string_chars( object *o );

// Byte based comparison. Returns 0 on match, -1 if the second string is
// 'bigger', 1 if the first string is 'bigger'
// 'bigger' is defined as the first differing byte is bigger in the given
// string.
// If the strings are identical, except that one is longer, the longer one is
// bigger.
int string_compare( object *a, object *b );

// Get an iterator that canbe used in the iteration functions below.
string_iterator *iterate_string( object *s );
void free_string_iterator( string_iterator *i );

// The unic variant returns the unicode codepoint
// byte variant returns a single byte.
// You cannot mix functions with a single iterator
ucs4_t   iterator_next_unic( string_iterator **i );
uint8_t  iterator_next_byte( string_iterator **i );

// Check if the iterator is finished
uint8_t  iterator_complete( string_iterator *i );

#endif
