#ifndef GSD_DICT_ERROR_H
#define GSD_DICT_ERROR_H

#include <stdint.h>

typedef union {
    // For quick checks, if .num == 0 you know everything is good if it is not
    // 0 then you need to check deeper to see if it was a failure, error, or
    // rebalance error.
    uint32_t num;

    struct {
        // Set to true if the operation failed. A failure is not the same as an
        // error. For example an insert will fail if the key is already in the
        // hash, this is desired behavior for a transactional system.
        unsigned int fail : 8;

        // Set to true only if there was an error during rebalance
        unsigned int rebal : 8;

        // Category or type of error
        enum {
            DICT_NO_ERROR = 0,
            DICT_PATHOLOGICAL,
            DICT_OUT_OF_MEMORY,
            DICT_API_MISUSE,
            DICT_UNIMPLEMENTED,
            DICT_UNKNOWN
        } error : 8;

        // Index for error text
        uint16_t message_idx : 8;
    } bit;
} dict_stat;

extern char *dict_messages[];

char *dict_stat_message( dict_stat *s );

#endif
