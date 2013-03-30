#include <assert.h>
#include <string.h>

#include "parser.h"

char_type get_char_type( char_type prefix, uint8_t *c ) {
    return WHITESPACE;
}

presub *parse_text( object *to, dict *scope, uint8_t *code ) {
    // Copy code pointer for iteration
    uint8_t *ci = code;

    thread *t = to->data;
    instance *i = t->instance;

    size_t token_count = 100;
    size_t token_index = 0;
    token *tokens = malloc( sizeof( token ) * token_count );
    assert( tokens );
    memset( tokens, 0, sizeof( token )); // set first token to 0
    while ( *ci != '\0' ) {
        char_type t = get_char_type( tokens[token_index].type, ci );

        // skip initial whitespace, create token
        if ( !tokens[token_index].start && t != WHITESPACE ) {
            tokens[token_index].type   = t;
            tokens[token_index].start  = ci;
            tokens[token_index].length = 0;
            tokens[token_index].symbol = NULL;
        }
        // Time for a new token
        else if ( t != tokens[token_index].type ) {
            token *cur = &tokens[token_index];
            cur->length = ci - cur->start;
            switch ( cur->type ) {
                // Numeric is a constant
                case NUMERIC:
                    cur->scalar = create_scalar( to, SET_FROM_STRL_NUM, cur->start, cur->length );
                break;

                case NEWLINE:
                break;

                default:
                    // Make a scalar for the symbol
                    cur->scalar = create_scalar( to, SET_FROM_STRL, cur->start, cur->length );
                    assert( cur->scalar );

                    // Find symbol
                    dict_stat s = dict_get( scope, cur->scalar, (void **)&(cur->symbol));
                    assert( !s.bit.error );

                    // if it is non-alphanumeric, and no symbol: exception
                    if ( t == SYMBOLIC ) assert( cur->symbol );
                    if ( cur->symbol && cur->symbol->type == i->keyword_t ) {
                        // TODO: Run keyword
                        abort();
                    }
                break;
            }

            token_index++;
            if ( token_index >= token_count ) {
                token_count += 100;
                token *new = realloc( tokens, token_count * sizeof( token ));
                assert( new );
                tokens = new;
            }

            memset( &(tokens[token_index]), 0, sizeof( token ));
        }

        ci++;
    }

    // Tokenized! time to compile
}
