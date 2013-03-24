#ifndef GSD_BYTECODE_H
#define GSD_BYTECODE_H
#include <stdlib.h>
#include <stdint.h>
#include "GSD_Dict/src/include/gsd_dict.h"

#include "structure.h"

#define OP_END     0
#define OP_CALL    1
#define OP_SET     2
#define OP_APUSH   3
#define OP_PUSH    4
#define OP_PUSHREF 5
#define OP_REF     6
#define OP_MAP     7
#define OP_GRAPH   8

typedef uint8_t subop;

typedef struct oparg  oparg;
typedef struct presub presub;
typedef struct arg_list arg_list;
typedef struct bytecode bytecode;

struct oparg {
    unsigned int lookup : 1;
    enum {
        SRC_RET       = 0,
        SRC_EXCEPTION = 1,
        SRC_DATA      = 2,
        SRC_INST      = 3,
        SRC_COND      = 4,
    } source : 7;
};

struct bytecode {
    subop   *entry;
    object **data;

    size_t entry_size;
    size_t entry_idx;

    size_t data_size;
    size_t data_idx;
};

struct presub {
    dict *closures;
    dict *symbols;

    subop   *entry;
    object **data;

    scalar_string *filename;
    size_t start_line;
    size_t end_line;

    size_t refcount;
};

struct arg_list {
    arg_list *parent;
    object   *inst;
    dict     *args;
    int64_t  push_idx;
};

presub *bc_to_presub( bytecode *bc );

bytecode *new_bytecode();

int bc_push_op( bytecode *bc,   uint8_t op );
int bc_push_arg( bytecode *bc,  unsigned int l, int source );
int bc_push_data( bytecode *bc, object *o );


#endif

