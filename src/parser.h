#ifndef GSD_PARSER_H
#define GSD_PARSER_H

#include <stdlib.h>
#include "bytecode.h"
#include "structure.h"
#include "instance.h"
#include "types.h"
#include "memory.h"

typedef struct token token;

typedef enum {
    WHITESPACE = 0,
    NUMERIC,
    SYMBOLIC,
    ALPHANUMERIC,
    NEWLINE,
} char_type;

struct token {
    uint8_t *start;
    char_type type;
    unsigned int length;
    object *scalar;
    object *symbol;
};

presub *parse_text( object *thread, dict *scope, uint8_t *code );

#endif
