#include <assert.h>
#include <string.h>

#include "parser.h"

parser_char parser_getc( parser *p ) {
    assert( p->buffer || p->fp );

    parser_char c = { NULL, 0, WHITESPACE };

    if ( p->buffer ) {
        c.start = p->buffer + p->buffer_idx;
        printf( "C: %c\n", c.start[0] );

        // TODO: Unicode
        c.length = 1;
        p->buffer_idx += c.length;

        switch( c.start[0] ) {
            case '\n':
                c.type = NEWLINE;
            break;

            case ' ':
            case '\t':
            case '\r':
                c.type = WHITESPACE;
            break;

            case '_':
                c.type = ALPHANUMERIC;
            break;

            default:
                if ( c.start[0] < 33 ) {
                    fprintf( stderr, "Invalid character: [%u]\n", c.start[0] );
                    abort();
                }
                else if ( c.start[0] < 48 ) {
                    c.type = SYMBOLIC;
                }
                else if ( c.start[0] < 58 ) {
                    c.type = ALPHANUMERIC;
                }
                else if ( c.start[0] < 65 ) {
                    c.type = SYMBOLIC;
                }
                else if ( c.start[0] < 90 ) {
                    c.type = ALPHANUMERIC;
                }
                else if ( c.start[0] < 97 ) {
                    c.type = SYMBOLIC;
                }
                else if ( c.start[0] < 122 ) {
                    c.type = ALPHANUMERIC;
                }
                else if ( c.start[0] < 127 ) {
                    c.type = SYMBOLIC;
                }
                else {
                    // TODO: Unicode
                    fprintf( stderr, "Invalid character: [%u]\n", c.start[0] );
                    abort();
                }
            break;
        }

        return c;
    }

    abort();
}

presub *parse_text( object *to, dict *scope, uint8_t *code ) {
    thread *t = to->data;
    instance *i = t->instance;

    parser p = { NULL, code, 0, strlen( (char *)code ), NULL, NULL, 100, 0 };
    p.tokens = malloc( sizeof( token ) * p.token_count );
    assert( p.tokens );
    p.token = p.tokens;
    memset( p.token, 0, sizeof( token )); // set first token to 0

    parser_char c = parser_getc( &p );
    while ( c.start ) {
        if ( !p.token->start ) {
            p.token->start  = c.start;
            p.token->size   = 1;
            p.token->type   = c.type;
            p.token->symbol = NULL;

            // Next character
            c = parser_getc( &p );
            continue;
        }

        // Same type, token continues
        if ( p.token->type == c.type ) {
            c = parser_getc( &p );
            continue;
        }

        // Token boundry
        p.token->size = c.start - p.token->start;

        if ( p.token->type > NEWLINE ) {
            // TODO: Unicode
            if ( p.token->start[0] > 47 && p.token->start[0] < 58 ) {
                p.token->scalar = create_scalar( to, SET_FROM_STRL_NUM, p.token->start, p.token->size );
            }
            else {
                // Make a scalar for the symbol
                p.token->scalar = create_scalar( to, SET_FROM_STRL, p.token->start, p.token->size );
                assert( p.token->scalar );

                // Find symbol
                dict_stat s = dict_get( scope, p.token->scalar, (void **)&(p.token->symbol));
                assert( !s.bit.error );

                // if it is non-alphanumeric, and no symbol: exception
                if ( p.token->type == SYMBOLIC ) {
                    printf( "S-Token start: %c\n", p.token->start[0] );
                    assert( p.token->symbol );
                }
                if ( p.token->symbol && p.token->symbol->type == i->keyword_t ) {
                    // TODO: Run keyword
                    abort();
                }
            }
        }

        printf( "Token Complete: %c, %zu\n", p.token->start[0], p.token->size );

        p.token_index++;
        if ( p.token_index >= p.token_count ) {
            p.token_count += 100;
            token *new = realloc( p.tokens, p.token_count * sizeof( token ));
            assert( new );
            p.tokens = new;
            p.token = p.tokens + p.token_index;
        }

        // Set next token to 0
        memset( p.token, 0, sizeof( token ));
    }

    // Tokenized! time to compile
}
