#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../include/string_api.h"
#include "../structures.h"
#include "../string.h"

int main() {
    object_simple *snip1 = malloc(sizeof(object_simple));

    snip1->object.type      = GC_SNIP;
    snip1->object.state     = GC_UNCHECKED;
    snip1->object.permanent = 0;
    snip1->object.epoch     = 0;
    snip1->object.ref_count = 1;

    snip1->simple_data.snip.bytes = 6;
    snip1->simple_data.snip.chars = 5;
    snip1->simple_data.snip.data[0] = 'a';
    snip1->simple_data.snip.data[1] = 'b';
    snip1->simple_data.snip.data[2] = 'c';
    snip1->simple_data.snip.data[3] = 'd';
    snip1->simple_data.snip.data[4] = 0xC7;
    snip1->simple_data.snip.data[5] = 0xBF;

    assert( string_bytes( &(snip1->object) ) == 6 );
    assert( string_chars( &(snip1->object) ) == 5 );

    string_iterator *i = iterate_string(&(snip1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 'b' );
    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 'c' );
    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 0xC7 );
    assert( !iterator_complete(i) );

    assert( iterator_next_byte(&i) == 0xBF );
    assert( iterator_complete(i) );

    assert( iterator_next_byte(&i) == 0 );

    free_string_iterator( i );
    i = iterate_string(&(snip1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'b' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'c' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0x01FF );
    assert( iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0 );

    free_string_iterator( i );
    i = iterate_string(&(snip1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_utf8(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_utf8(&i) == 'b' );
    assert( !iterator_complete(i) );

    assert( iterator_next_utf8(&i) == 'c' );
    assert( !iterator_complete(i) );

    assert( iterator_next_utf8(&i) == 'd' );
    assert( !iterator_complete(i) );

    union {
        uint8_t  parts[4];
        uint32_t whole;
    } got;

    union {
        uint8_t  parts[4];
        uint32_t whole;
    } want;

    want.parts[0] = 0xC7;
    want.parts[1] = 0xBF;
    want.parts[2] = 0;
    want.parts[3] = 0;
    got.whole = iterator_next_utf8(&i);

    assert( got.whole == want.whole );
    assert( iterator_complete(i) );

    assert( iterator_next_utf8(&i) == 0 );


    printf( "No errors!\n" );
}
