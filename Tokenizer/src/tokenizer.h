#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "include/gsd_tokenizer_api.h"
#include <unictype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

typedef struct source source;
typedef struct char_info char_info;

struct source {
    uint8_t *buffer;
    size_t   size;
    size_t   index;
    FILE    *fp;
};

struct char_info {
    uint8_t size : 4;
    enum {
        CHAR_INVALID = 0,
        CHAR_NEWLINE,      // '\n'
        CHAR_WHITESPACE,   // Z
        CHAR_ALPHANUMERIC, // L or Nd
        CHAR_SYMBOL,       // P or S or M or Nl or No
        CHAR_CONTROL,      // C
    } type : 4;
};

char_info get_char_info(uint8_t *start, size_t size);

#endif
