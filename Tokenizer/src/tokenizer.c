#include <unictype.h>
#include <unistr.h>
#include <unitypes.h>
#include <assert.h>

#include "include/gsd_tokenizer_api.h"
#include "tokenizer.h"

token_set *tokenize_cstring( uint8_t *input );
token_set *tokenize_string ( uint8_t *input,    size_t size );
token_set *tokenize_file   ( uint8_t *filename, size_t size );

token *token_set_next( token_set *s );

char_info get_char_info(uint8_t *start, size_t size) {
    char_info out = {
        .size = 0,
        .type = CHAR_INVALID,
    };

    if (!size)  return out;
    if (!start) return out;

    ucs4_t it = 0;
    uint8_t length = u8_mbtouc(&it, start, size);
    assert( length <= 4 );

    switch(it) {
        case '\n':
            out.type = CHAR_NEWLINE;
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
        else if (uc_is_general_category(it, UC_CATEGORY_Nd)) {
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
