#ifndef GSD_PARSER_H
#define GSD_PARSER_H

#include <stdlib.h>
#include "bytecode.h"
#include "structure.h"
#include "instance.h"
#include "types.h"
#include "memory.h"

parser_char parser_getc( parser *p );
void parser_petc( parser *p, parser_char c );

token parser_get_token( parser *p );

token parser_pop_token( parser *p );
token parser_peek_token( parser *p );
int parser_push_token( parser *p, token t );

presub *parse_text( object *thread, dict *scope, uint8_t *code );

statement *parser_shift_statement( parser *p );

#endif
