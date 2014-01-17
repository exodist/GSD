#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unictype.h>
#include <unistdio.h>
#include <unistr.h>
#include <unitypes.h>

#include "include/gsd_tokenizer_api.h"
#include "tokenizer.h"

#define SOURCE_BUFFER_SIZE 1024
#define GROUP_SIZE  1024
#define GROUP_COUNT 1024
#define TOKEN_SIZE  16

token_set *tokenize_stream ( FILE *fp ) {

}

token_set *tokenize_cstring( uint8_t *input );
token_set *tokenize_string ( uint8_t *input,    size_t size );
token_set *tokenize_file   ( char *filename );

token_set *tokenize_source( source *s ) {
    token_set *ts = malloc(sizeof(token_set));
    if (!ts) return NULL;
    memset( ts, 0, sizeof(token_set) );

    char_info last = { 0 };
    while (1) {
        char_info inf = source_get_char(s);
        if (!inf.size) {
            ts->error      = inf.error;
            ts->file_error = inf.file_error;
            return ts;
        }

        // advance to next token if necessary
        if (last.size) {
            token *t = ts->tokens[ts->store_group] + ts->store_index;
            if (inf.type == t->type) {
                // No Op
            }
            else if (last.type != TOKEN_SPACE && inf.type == TOKEN_COMBINE) {
                // No Op
            }
            else if (uc_is_general_category(t->start[0], UC_CATEGORY_N) && inf.ucs4 == '.') {
                // No Op
            }
            else {
                ts->store_index++;
                if (ts->store_index >= ts->group_size) {
                    ts->store_index = 0;
                    ts->store_group++;
                }
            }
        }

        // Init groups if needed
        if (!ts->tokens || ts->store_group >= ts->group_count) {
            if( !ts->group_size ) ts->group_size = GROUP_SIZE;

            ts->group_count += GROUP_COUNT;
            void *new_tokens = realloc(ts->tokens, sizeof(token *) * ts->group_count);
            if (!new_tokens) {
                free_token_set(ts);
                return NULL;
            }
            ts->tokens = new_tokens;
            memset( ts->tokens + ts->store_group, 0, sizeof(token *) * GROUP_COUNT );
        }

        // Init group  if needed
        if (!ts->tokens[ts->store_group]) {
            ts->tokens[ts->store_group] = malloc(sizeof(token) * ts->group_size);
            if (!ts->tokens[ts->store_group]) {
                free_token_set(ts);
                return NULL;
            }
            memset( ts->tokens[ts->store_group], 0, sizeof(token) * ts->group_size );
        }

        // Init token if needed
        token *t = ts->tokens[ts->store_group] + ts->store_index;
        if (!t->start) {
            t->type = inf.type;
        }

        if(!t->start || t->size + 1 >= t->buffer_size) {
            t->buffer_size += TOKEN_SIZE;
            void *new_buffer = realloc(t->start, sizeof(ucs4_t) * t->buffer_size);
            if (!new_buffer) {
                free_token_set(ts);
                return NULL;
            }
            t->start = new_buffer;
        }

        // Append character to token
        t->start[t->size] = inf.ucs4;
        t->size++;

        last = inf;
    }

    return ts;
}

char_info source_get_char(source *s) {
    char_info out = {
        .ucs4  = 0,
        .type  = TOKEN_INVALID,
        .error = TOKEN_ERROR_NONE,
        .file_error = 0,
        .size = 0,
    };

    if (s->fp && !feof(s->fp) && s->size - s->index < 4) {
        // Create buffer if needed
        if (!s->buffer) {
            s->buffer = malloc(SOURCE_BUFFER_SIZE);
            if (!s->buffer) {
                out.error = TOKEN_ERROR_EOM;
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
        if (got < SOURCE_BUFFER_SIZE - s->size && ferror(s->fp)) {
            out.error = TOKEN_ERROR_FILE;
            out.file_error = errno;
            return out;
        }
        s->size += got;
        assert( s->size <= SOURCE_BUFFER_SIZE );
    }

    char_info i = get_char_info(s->buffer + s->index, s->size - s->index);
    s->index += i.size;
    return i;
}

char_info get_char_info(uint8_t *start, size_t size) {
    char_info out = {
        .ucs4 = 0,
        .type = TOKEN_INVALID,
        .error = TOKEN_ERROR_NONE,
        .file_error = 0,
        .size = 0,
    };

    if (!size)  return out;
    if (!start) return out;

    ucs4_t it = 0;
    uint8_t length = u8_mbtouc(&it, start, size);
    assert( length <= 4 );
    out.ucs4 = it;
    out.size = length;

    int combining = uc_combining_class(it);
    if (combining) {
        out.type = TOKEN_COMBINE;
    }

    switch(it) {
        case '\n':
            out.type = TOKEN_NEWLINE;
        break;

        case ';':
            out.type = TOKEN_TERMINATE;
        break;

        case '_':
            out.type = TOKEN_ALPHANUM;
        break;

        case ' ':
        case '\t':
        case '\r':
            out.type = TOKEN_SPACE;
        break;
    }

    if (out.type == TOKEN_INVALID) {
        if (uc_is_general_category(it, UC_CATEGORY_L)) {
            out.type = TOKEN_ALPHANUM;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_N)) {
            out.type = TOKEN_ALPHANUM;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_Z)) {
            out.type = TOKEN_SPACE;
        }
        else if (uc_is_general_category(it, UC_CATEGORY_C)){
            out.type = TOKEN_CONTROL;
        }
        else {
            out.type = TOKEN_SYMBOL;
        }
    }

    return out;
}

token_set_iterator iterate_token_set( token_set *ts ) {
    token_set_iterator tsi = {
        .ts = ts,
        .group = 0,
        .index = 0,
    };
    return tsi;
}

token *ts_iter_next( token_set_iterator *tsi ) {
    if (!tsi->ts->tokens) return NULL;
    if (tsi->group >= tsi->ts->group_count) return NULL;
    if (!tsi->ts->tokens[tsi->group]) return NULL;

    token *t = tsi->ts->tokens[tsi->group] + tsi->index;
    if (t->size < 1) return NULL;

    tsi->index++;
    if (tsi->index >= tsi->ts->group_size) {
        tsi->index = 0;
        tsi->group++;
    }

    return t;
}

void dump_token_set( token_set *s ) {
    token_set_iterator tsi = iterate_token_set(s);
    token *t = ts_iter_next(&tsi);
    while(t) {
        char *out = NULL;
        switch (t->type) {
            case TOKEN_NEWLINE:   out = "NEWLINE";   break;
            case TOKEN_SPACE:     out = "SPACE";     break;
            case TOKEN_ALPHANUM:  out = "ALPHANUM";  break;
            case TOKEN_SYMBOL:    out = "SYMBOL";    break;
            case TOKEN_CONTROL:   out = "CONTROL";   break;
            case TOKEN_TERMINATE: out = "TERMINATE"; break;
            case TOKEN_INVALID:   out = "INVALID";   break;
            case TOKEN_COMBINE:   out = "COMBINE";   break;

            default: out = "OTHER";
        }
        printf( "%-9s (%zi): ", out, t->size );
        if (t->size < 10) printf(" ");
        if (t->size < 100) printf(" ");
        if (t->type == TOKEN_NEWLINE) {
            printf( "\\n" );
        }
        else if (t->type == TOKEN_CONTROL){
            for ( size_t i = 0; i < t->size; i++ ) {
                if (i > 0) printf( " " );
                printf( "%x", t->start[i] );
            }
        }
        else {
            if (t->type == TOKEN_COMBINE) printf( " " );
            for( size_t i = 0; i < t->size && i < 50; i++ ) {
                uint8_t buffer[4];
                int l = u8_uctomb(buffer, t->start[i], 4);
                fwrite( buffer, 1, l, stdout );
            }
            if (t->size > 50) printf( "..." );
        }
        printf( "\n" );

        t = ts_iter_next(&tsi);
    }
}

void free_token_set( token_set *ts ) {
    token_set_iterator tsi = iterate_token_set(ts);
    token *t = ts_iter_next(&tsi);
    while(t) {
        free( t->start);
        t = ts_iter_next(&tsi);
    }
    if (ts->tokens) {
        for (size_t i = 0; i < ts->group_count; i++) {
            if (!ts->tokens[i]) break;
            free( ts->tokens[i] );
        }
    }
    free(ts->tokens);
    free(ts);
}
