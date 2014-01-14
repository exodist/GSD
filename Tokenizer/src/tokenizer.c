#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <unictype.h>
#include <unistr.h>
#include <unitypes.h>
#include <stdio.h>

#include "include/gsd_tokenizer_api.h"
#include "tokenizer.h"

#define SOURCE_BUFFER_SIZE 1024

token_set *tokenize_cstring( uint8_t *input );
token_set *tokenize_string ( uint8_t *input,    size_t size );
token_set *tokenize_file   ( uint8_t *filename, size_t size );

token_set *tokenize_source( source *s ) {
    char_info i = source_get_char(s);
    assert( i.size );
    return NULL;
}

token *token_set_next( token_set *s );

char_info source_get_char(source *s) {
    if (s->fp && !feof(s->fp) && s->size - s->index < 4) {
        // Create buffer if needed
        if (!s->buffer) {
            s->buffer = malloc(SOURCE_BUFFER_SIZE);
            if (!s->buffer) {
                char_info out = { .ptr = NULL, .size = 0, .type = CHAR_INVALID };
                return out;
            }
        }

        if (s->size - s->index) {
            memmove( s->buffer, s->buffer + s->index, s->size - s->index );
            s->size -= s->index;
            s->index = 0;
        }

        // Do the read
        size_t got = fread( s->buffer + s->size, 1, SOURCE_BUFFER_SIZE - s->size, s->fp );
        s->size += got;
        assert( s->size <= SOURCE_BUFFER_SIZE );
    }

    char_info i = get_char_info(s->buffer + s->index, s->size - s->index);
    s->index += i.size;
    return i;
}

char_info get_char_info(uint8_t *start, size_t size) {
    char_info out = {
        .ptr  = start,
        .size = 0,
        .type = CHAR_INVALID,
    };

    if (!size)  return out;
    if (!start) return out;

    ucs4_t it = 0;
    uint8_t length = u8_mbtouc(&it, start, size);
    assert( length <= 4 );

    out.size = length;

    switch(it) {
        case '\n':
            out.type = CHAR_NEWLINE;
        break;

        case '_':
            out.type = CHAR_ALPHANUMERIC;
        break;

        case ' ':
        case '\t':
        case '\r':
            out.type = CHAR_WHITESPACE;
        break;
    }

    if (out.type == CHAR_INVALID) {
        if (uc_is_general_category(it, UC_CATEGORY_L)) {
            out.type = CHAR_ALPHANUMERIC;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_N)) {
            out.type = CHAR_ALPHANUMERIC;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_Z)) {
            out.type = CHAR_WHITESPACE;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_C)){
            out.type = CHAR_CONTROL;
        }
        else {
            out.type = CHAR_SYMBOL;
        }
    }

    return out;
}
