#ifndef GSD_TOKENIZER_API_H
#define GSD_TOKENIZER_API_H

#include <stdlib.h>
#include <stdio.h>

typedef struct token token;
typedef struct token_set token_set;
typedef struct token_set_iterator token_set_iterator;

typedef enum {
    TOKEN_ERROR_NONE = 0, // No error
    TOKEN_ERROR_EOM,      // Out of memory
    TOKEN_ERROR_FILE,     // Problem with file (see file_error, which is the errno)
} tokenizer_error;

typedef enum {
    TOKEN_INVALID = 0,
    TOKEN_NEWLINE,
    TOKEN_SPACE,
    TOKEN_ALPHANUM,
    TOKEN_SYMBOL,
    TOKEN_CONTROL,
    TOKEN_TERMINATE,
    TOKEN_COMBINE,
} token_type;

struct token_set {
    token **tokens;
    size_t  group_count;
    size_t  group_size;

    size_t  store_group;
    size_t  store_index;

    tokenizer_error error;
    size_t file_error;
};

struct token_set_iterator {
    token_set *ts;
    size_t group;
    size_t index;
};

struct token {
    uint8_t *start;
    size_t   buffer_size;
    size_t   size;

    token_type type;

    ucs4_t initial;
};

token_set *tokenize_stream ( FILE *fp );
token_set *tokenize_file   ( char *filename );
token_set *tokenize_cstring( uint8_t *input );
token_set *tokenize_string ( uint8_t *input, size_t size );

token *ts_iter_next( token_set_iterator *tsi );

token_set_iterator iterate_token_set( token_set *ts );

void free_token_set( token_set *s );

void dump_token_set( token_set *s );

#endif
