#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/string_api.h"
#include "../structures.h"
#include "../string.h"

void test_snip();
void test_stringc();
void test_string();
void test_rope();
void test_compare();

int main() {
    test_snip();
    test_stringc();
    test_string();
    test_rope();
    test_compare();

    printf( "No errors!\n" );
}

void test_snip() {
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

    snip1->simple_data.snip.data[2] = 0xF5;
    i = iterate_string(&(snip1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'b' );
    assert( !iterator_complete(i) );

    // invalid character, should be replaced
    assert( iterator_next_unic(&i) == 0xFFFD );
    assert( !iterator_complete(i) );

    // Still iterated after invalid character
    assert( iterator_next_unic(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0x01FF );
    assert( iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0 );

    free_string_iterator( i );
}

void test_stringc() {
    string_const sc = {
        .head = { .bytes = 6, .chars = 5, .hash = 0 },
        .string = (void *)"abcdǿ",
    };
    object_simple *string1 = malloc(sizeof(object_simple));

    string1->object.type      = GC_STRINGC;
    string1->object.state     = GC_UNCHECKED;
    string1->object.permanent = 0;
    string1->object.epoch     = 0;
    string1->object.ref_count = 1;
    string1->simple_data.ptr  = &sc;

    assert( string_bytes( &(string1->object) ) == 6 );
    assert( string_chars( &(string1->object) ) == 5 );

    string_iterator *i = iterate_string(&(string1->object));

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
    i = iterate_string(&(string1->object));

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

    sc.string = (uint8_t *)"ab" "\xF5" "dǿ";
    i = iterate_string(&(string1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'b' );
    assert( !iterator_complete(i) );

    // invalid character, should be replaced
    //printf( "---> %x\n", iterator_next_unic(&i) );
    assert( iterator_next_unic(&i) == 0xFFFD );
    assert( !iterator_complete(i) );

    // Still iterated after invalid character
    assert( iterator_next_unic(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0x01FF );
    assert( iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0 );

    free_string_iterator( i );
}

void test_string() {
    string *st = malloc( sizeof(string) + 6 );
    st->head.bytes = 6;
    st->head.chars = 5;
    uint8_t *bytes = &(st->first_byte);
    bytes[0] = 'a';
    bytes[1] = 'b';
    bytes[2] = 'c';
    bytes[3] = 'd';
    bytes[4] = 0xC7;
    bytes[5] = 0xBF;

    object_simple *string1 = malloc(sizeof(object_simple));
    string1->object.type      = GC_STRING;
    string1->object.state     = GC_UNCHECKED;
    string1->object.permanent = 0;
    string1->object.epoch     = 0;
    string1->object.ref_count = 1;
    string1->simple_data.ptr  = st;

    assert( string_bytes( &(string1->object) ) == 6 );
    assert( string_chars( &(string1->object) ) == 5 );

    string_iterator *i = iterate_string(&(string1->object));

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
    i = iterate_string(&(string1->object));

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

    bytes[2] = 0xF5;
    i = iterate_string(&(string1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'b' );
    assert( !iterator_complete(i) );

    // invalid character, should be replaced
    //printf( "---> %x\n", iterator_next_unic(&i) );
    assert( iterator_next_unic(&i) == 0xFFFD );
    assert( !iterator_complete(i) );

    // Still iterated after invalid character
    assert( iterator_next_unic(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0x01FF );
    assert( iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0 );

    free_string_iterator( i );
}

void test_rope() {
    object_simple snips[5] = {
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'a'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'b'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'c'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'d'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 2, .chars = 1, .data = {0xC7, 0xBF}}},
    };

    object *children[5] = {
        &(snips[0].object),
        &(snips[1].object),
        &(snips[2].object),
        &(snips[3].object),
        &(snips[4].object),
    };

    string_rope sr = {
        .head = { .bytes = 6, .chars = 5 },
        .child_count = 5,
        .children = children,
    };

    object_simple *string1 = malloc(sizeof(object_simple));
    string1->object.type      = GC_ROPE;
    string1->object.state     = GC_UNCHECKED;
    string1->object.permanent = 0;
    string1->object.epoch     = 0;
    string1->object.ref_count = 1;
    string1->simple_data.ptr  = &sr;

    assert( string_bytes( &(string1->object) ) == 6 );
    assert( string_chars( &(string1->object) ) == 5 );

    string_iterator *i = iterate_string(&(string1->object));

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
    i = iterate_string(&(string1->object));

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

    snips[2].simple_data.snip.data[0] = 0xF5;
    i = iterate_string(&(string1->object));

    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'a' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 'b' );
    assert( !iterator_complete(i) );

    // invalid character, should be replaced
    //printf( "---> %x\n", iterator_next_unic(&i) );
    assert( iterator_next_unic(&i) == 0xFFFD );
    assert( !iterator_complete(i) );

    // Still iterated after invalid character
    assert( iterator_next_unic(&i) == 'd' );
    assert( !iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0x01FF );
    assert( iterator_complete(i) );

    assert( iterator_next_unic(&i) == 0 );

    free_string_iterator( i );
}

void test_compare() {
    object_simple snip1 = {
        .object = { .type = GC_SNIP, .ref_count = 1 },
        .simple_data.snip = {
            .bytes = 4,
            .chars = 4,
            .data = { 'a', 'B', 'c', 'D' },
        }
    };

    string_const sc = {
        .head = { .bytes = 4, .chars = 4 },
        .string = (uint8_t *)"aBcD",
    };
    object_simple stringc1 = {
        .object = { .type = GC_STRINGC, .ref_count = 1 },
        .simple_data.ptr = &sc,
    };

    string *st = malloc(sizeof(string) + 3);
    st->head.bytes = 4;
    st->head.chars = 4;
    memcpy( &(st->first_byte), "aBcD", 4 );
    object_simple string1 = {
        .object = { .type = GC_STRING, .ref_count = 1 },
        .simple_data.ptr = st,
    };

    object_simple snips[4] = {
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'a'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'B'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'c'}}},
        {.object = {.type = GC_SNIP, .ref_count = 1}, .simple_data.snip = {.bytes = 1, .chars = 1, .data = {'D'}}},
    };
    object *children[4] = {
        &(snips[0].object),
        &(snips[1].object),
        &(snips[2].object),
        &(snips[3].object),
    };
    string_rope sr = {
        .head = { .bytes = 4, .chars = 4 },
        .child_count = 4,
        .children = children,
    };
    object_simple rope1 = {
        .object = { .type = GC_ROPE, .ref_count = 1 },
        .simple_data.ptr = &sr,
    };

    assert( string_compare( &(snip1.object),    &(stringc1.object) ) == 0 );
    assert( string_compare( &(snip1.object),    &(string1.object)  ) == 0 );
    assert( string_compare( &(snip1.object),    &(rope1.object)    ) == 0 );
    assert( string_compare( &(stringc1.object), &(string1.object)  ) == 0 );
    assert( string_compare( &(stringc1.object), &(rope1.object)    ) == 0 );
    assert( string_compare( &(string1.object),  &(rope1.object)    ) == 0 );

    // Make the snip bigger in the last character
    snip1.simple_data.snip.data[3] = 'E';
    assert( string_compare( &(snip1.object), &(stringc1.object) ) ==  1 );
    assert( string_compare( &(stringc1.object), &(snip1.object) ) == -1 );

    // Make the snip the same again
    snip1.simple_data.snip.data[3] = 'D';
    assert( string_compare( &(snip1.object), &(stringc1.object) ) == 0 );

    // Add a character (null) to the snip, it is now longer, bigger.
    snip1.simple_data.snip.data[4] = '\0';
    snip1.simple_data.snip.bytes = 5;
    snip1.simple_data.snip.chars = 5;
    assert( string_compare( &(snip1.object), &(stringc1.object) ) ==  1 );
    assert( string_compare( &(stringc1.object), &(snip1.object) ) == -1 );
}
