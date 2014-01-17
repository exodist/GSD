#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "include/gsd_tokenizer_api.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct source source;
typedef struct char_info char_info;

struct source {
    uint8_t *buffer;
    size_t   size;
    size_t   index;
    FILE     *fp;
};

struct char_info {
    ucs4_t ucs4;
    uint8_t size;

    token_type type;

    tokenizer_error error;
    size_t file_error;
};

char_info source_get_char(source *s);

char_info get_char_info(uint8_t *start, size_t size);

token_set *tokenize_source( source *s );

#endif
