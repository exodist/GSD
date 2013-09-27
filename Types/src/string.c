#include "include/string_api.h"
#include "include/static_api.h"
#include "include/object_api.h"
#include "string.h"
#include "structures.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

size_t string_bytes( object *s ) {
    assert( s->ref_count );
    assert( s->type == GC_STRING || s->type == GC_ROPE || s->type == GC_SNIP );

    switch (s->type) {
        case GC_STRING:
        case GC_ROPE:
        return ((string_header *)s)->bytes;

        case GC_SNIP:
        return ((string_snip *)s)->bytes;
    }
}

size_t string_chars( object *s ) {
    assert( s->ref_count );
    assert( s->type == GC_STRING || s->type == GC_ROPE || s->type == GC_SNIP );

    switch (s->type) {
        case GC_STRING:
        case GC_ROPE:
        return ((string_header *)s)->chars;

        case GC_SNIP:
        return ((string_snip *)s)->chars;
    }
}

int string_compare( object *a, object *b ) {
    assert( a->ref_count && b->ref_count );
    assert( a->type == GC_STRING || a->type == GC_ROPE || a->type == GC_SNIP );
    assert( b->type == GC_STRING || b->type == GC_ROPE || b->type == GC_SNIP );

    string_iterator *ia = iterate_string(a);
    string_iterator *ib = iterate_string(b);

    size_t bytes_a = string_bytes(a);
    size_t bytes_b = string_bytes(b);
    size_t bytes = bytes_a < bytes_b ? bytes_a : bytes_b;

    for(size_t i = 0; i < bytes; i++) {
        uint8_t ca = iterator_next_byte(ia);
        uint8_t cb = iterator_next_byte(ib);

        if (ca == cb) continue;
        if (ca >  cb) return  1;
        if (ca <  cb) return -1;
    }

    if (bytes_a > bytes_b) return  1;
    if (bytes_b < bytes_a) return -1;

    return 0;
}

string_iterator *iterate_string( object *s ) {
    assert( s->ref_count );
    assert( s->type == GC_STRING || s->type == GC_ROPE || s->type == GC_SNIP );

    string_iterator *i = malloc(sizeof(string_iterator));
    assert( i );
    memset( i, 0, sizeof(string_iterator) );
    i->item = s;

    return i;
}

uint8_t iterator_next_byte( string_iterator *i ) {
    
}

uint32_t iterator_next_utf8( string_iterator *i );
uint32_t iterator_next_unic( string_iterator *i );

