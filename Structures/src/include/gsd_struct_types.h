#ifndef GSD_STRUCT_TYPES_H
#define GSD_STRUCT_TYPES_H

#include <stdint.h>
#include <stdlib.h>

typedef struct dict   dict;
typedef struct sdict  sdict;
typedef struct ref    ref;
typedef struct array  array;
typedef struct bloom  bloom;
typedef struct sbloom sbloom;

typedef struct result result;

typedef uint64_t(hasher)(const void *item, void *meta);

typedef const char *(trigger)(void *arg, void *value, void *meta);

typedef void (refdelta)(void *item, int64_t delta);

typedef enum {
    RES_NO_ERROR = 0,
    RES_PATHOLOGICAL = 1,
    RES_OUT_OF_MEMORY,
    RES_API_MISUSE,
    RES_UNIMPLEMENTED,
    RES_IMMUTABLE,
    RES_TRIGGER_BLOCKED,
    RES_NO_TRIGGER,
    RES_UNKNOWN
} res_error;

// true value is success
//  0 means complete failure
// +1 means complete success
// -1 means success, but a rebalance was attempted and failed.
int res_check(result r);

// Returns the error code
res_error res_ecode(result r);

// Returns the error message (Not necessarily tied to the code)
const char *res_error_message(result r);

// For operations to fetch a value, this gets the object returned.
void *res_fetch(result r);

#endif
