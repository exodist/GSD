#ifndef GSD_STRUCT_RESULT_H
#define GSD_STRUCT_RESULT_H

#include <stdint.h>
#include <stdlib.h>
#include "gsd_struct_types.h"

typedef enum {
    RES_NO_ERROR = 0,
    RES_PATHOLOGICAL = 1,
    RES_OUT_OF_MEMORY,
    RES_API_MISUSE,
    RES_UNIMPLEMENTED,
    RES_IMMUTABLE,
    RES_TRIGGER_BLOCKED,
    RES_INVALID_NULL_VALUE,
    RES_BLOCKED,
    RES_UNKNOWN
} res_error_code;

typedef enum {
    RES_FAILURE       = 0, // Operation failed
    RES_SUCCESS       = 1, // Operation succeded

    // Operation succeded, but a non-critical secondary operation failed, for
    // example if an attempt to rebalance a tree after an insert fails.
    RES_SUCCESS_FUZZY = 2,
} res_status_code;

struct result {
    // If these are both 0 then the operation failed, but there was no error.
    // This happens when failure is an acceptable result. An example would be
    // if an insert into a dict fails because the key is already present, this
    // is a blocked transaction, but no error has actually occured, it is
    // working as designed.
    res_status_code status; // True on success
    res_error_code  error;  // True on error

    // Sometimes an error will come with a helpful message, this can be NULL.
    const char *message;

    enum {
        RESULT_ITEM_NONE = 0,
        RESULT_ITEM_REF,
        RESULT_ITEM_PTR,
        RESULT_ITEM_NUM
    } item_type;

    union {
        void    *ptr;
        ref     *ref;
        int64_t  num;
    } item;
};

void      result_discard(result r);
ref      *result_get_ref(result r);
int64_t   result_get_num(result r);
object   *result_get_obj(result r);

#endif
