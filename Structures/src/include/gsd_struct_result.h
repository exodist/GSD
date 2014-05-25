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

res_error_code result_error(result r);
res_status_code result_status(result r);
const char *result_message(result r);

void      result_discard(result r);
ref      *result_get_ref(result r);
int64_t   result_get_num(result r);
object   *result_get_obj(result r);

#endif
