#ifndef GSD_DICT_ERROR_H
#define GSD_DICT_ERROR_H

#include <stdint.h>

typedef union {
    uint32_t num;

    struct {
        unsigned int trans : 1;
        unsigned int error : 1;
        unsigned int rebal : 1;
        unsigned int patho : 1;

        enum {
            DICT_NO_ERROR = 0,
            DICT_OUT_OF_MEMORY,
            DICT_API_MISUSE,
            DICT_UNIMPLEMENTED,
            DICT_UNKNOWN
        } code : 12;

        uint16_t message_idx : 16;
    } bit;
} dict_stat;

#endif
