#include "include/string_api.h"
#include "string.h"
#include "structures.h"
#include <assert.h>

string_iterator iterate_string( object *s );

uint8_t  iterator_next_utf8( string_iterator *i );
uint32_t iterator_next_unic( string_iterator *i );
uint8_t  iterator_next_byte( string_iterator *i );

// Byte based comparison.
int string_compare( object *a, object *b );
