#ifndef GSD_PARSER_H
#define GSD_PARSER_H

#include <stdlib.h>
#include "bytecode.h"
#include "structure.h"
#include "instance.h"
#include "types.h"
#include "memory.h"

typedef struct token token;
typedef struct parser parser;
typedef struct parser_char parser_char;

typedef enum {
    WHITESPACE = 0,
    NEWLINE    = 1,
    ALPHANUMERIC,
    SYMBOLIC,
} char_type;

struct parser_char {
    uint8_t *start;
    uint8_t  length;
    char_type type;
};

struct parser {
    FILE    *fp;
    uint8_t *buffer;
    size_t   buffer_idx;
    size_t   buffer_length;

    parser_char put;

    token *token;
    token *tokens;
    size_t token_count;
    size_t token_index;
};

struct token {
    uint8_t *start;
    size_t    size;
    char_type type;
    object *scalar;
    object *symbol;
};

parser_char parser_getc( parser *p );
void parser_petc( parser *p, parser_char c );
token parser_get_token( parser *p );
presub *parse_text( object *thread, dict *scope, uint8_t *code );

#endif
