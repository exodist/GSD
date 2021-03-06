#ifndef GSD_STRUCTURES_H
#define GSD_STRUCTURES_H

#include <stdlib.h>
#include <stdint.h>


/* dict_stat: a return type
 * Most operations on the dictionary return a 'dict_stat'. A dict stat in a
 * union, it can be treated as an integer (.num) or as a bit-field structure
 * (.bit).
 *
 * If the numeric value is 0, that means there were no problems, the operation
 * completed successfully.
 *
 * Field explanation
 * fail  - If this is true the operation failed, if it is false then the
 *         operation succeeded. The contents of other fields have no effect
 *         here, even if there was an error you can trust that if this field is
 *         false that your operation succeeded.
 * rebal - Set to true when the error occured during a rebalance AFTER the
 *         operation succeeded.
 * error - The type of error that occured, 0 means no error occured.
 *         100+ are for external application use
 *
\*/

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
            DICT_PATHOLOGICAL = 1,
            DICT_OUT_OF_MEMORY,
            DICT_API_MISUSE,
            DICT_UNIMPLEMENTED,
            DICT_IMMUTABLE,
            DICT_TRIGGER,
            DICT_UNKNOWN
        } error : 8;

        const char *message;
    } bit;
} dict_stat;

typedef struct get_stat get_stat;

struct get_stat {
    dict_stat stat;
    void      *got;
};

const char *dict_stat_message( dict_stat s );

#endif
