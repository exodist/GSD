#ifndef GSD_GC_EXCEPTIONS_API_H
#define GSD_GC_EXCEPTIONS_API_H

typedef struct object object;

typedef enum {
    EX_NONE = 0,
    EX_OUT_OF_MEMORY,
    EX_NO_ATTRIBUTES,
    EX_NOT_A_TYPE,
    EX_DEC_AS_HKEY,
    EX_INVALID_HKEY,
    EX_NOT_IMPLEMENTED,
} exception;

typedef struct {
    exception e;
    object   *r;
} return_set;

#endif
