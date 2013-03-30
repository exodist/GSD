#ifndef GSD_TYPES_H
#define GSD_TYPES_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

#include "structure.h"
#include "bytecode.h"

typedef struct io io;
typedef struct type type;
typedef struct graph graph;
typedef struct object object;
typedef struct scalar scalar;
typedef struct thread thread;
typedef struct keyword keyword;
typedef struct subroutine subroutine;
typedef struct stack_frame stack_frame;

typedef object *(cfunction)( object *thread, stack_frame *f, object **exception );

struct scalar {
    int64_t   integer;
    double    decimal;

    scalar_string *string;

    uint8_t int_set;
    uint8_t dec_set;
    enum {
        SET_AS_INT,
        SET_AS_DEC,
        SET_AS_STR
    } init_as : 8;
};

struct io {
    FILE *fp;
};

struct graph {
    object *key;
    object *val;
};

struct type {
    object *name;
    object *parent;
    dict   *symbols;
    dict   *roles;

    object *call;

    uint8_t refcounted;
};

struct subroutine {
    struct presub *presub;
    dict *closures;
    dict *symbols;
};

struct stack_frame {
    stack_frame *parent;

    object *call;
    dict   *symbols;

    arg_list *args;
    object *condition;

    object *line_num;

    subop *subop;
    object **data;

    int complete;
};

struct thread {
    pthread_t pthread;
    instance *instance;

    arg_list *arg_stack;
    stack_frame *sub_stack;
    object *retval;
    object *exception;

    object *alloc;
};

struct keyword {
    int foo;
};

object *create_scalar( object *t, scalar_init vt, ... );

int init_io( instance *i );

object *io_call( object *thread, stack_frame *sf, object **exception );


#endif
