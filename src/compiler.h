#ifndef GSD_COMPILER_H
#define GSD_COMPILER_H

#include "structure.h"
#include "types.h"
#include "bytecode.h"

typedef struct statement statement;

struct statement {
    token *start;
    size_t count;
};

presub *compile_tokens( parser *p );

#endif
