#ifndef GSD_TOKENIZER_API_H
#define GSD_TOKENIZER_API_H

#include <stdlib.h>

typedef struct token token;
typedef struct token_set token_set;

struct token_set {
    token **tokens;
    size_t  group_count;
    size_t  group_size;

    size_t  iter_group;
    size_t  iter_index;

    size_t  store_group;
    size_t  store_index;
};

struct token {
    uint8_t *start;
    size_t  *size;

    enum {
        TOKEN_INVALID = 0,
        TOKEN_NEWLINE,
        TOKEN_SPACE,
        TOKEN_ALPHANUM,
        TOKEN_SYMBOL,
        TOKEN_CONTROL,
    } type;
};

token_set *tokenize_cstring( uint8_t *input );
token_set *tokenize_string ( uint8_t *input,    size_t size );
token_set *tokenize_file   ( uint8_t *filename, size_t size );

token *token_set_next( token_set *s );

#endif
