#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

typedef struct type type;
typedef struct graph graph;
typedef struct object object;
typedef struct scalar scalar;
typedef struct io io;
typedef struct thread thread;
typedef struct subroutine subroutine;
typedef struct stack_frame stack_frame;

typedef struct subop subop;
typedef struct arg_list arg_list;
typedef struct instance instance;
typedef struct dict_meta dict_meta;
typedef struct scalar_string scalar_string;

typedef object *(cfunction)( object *thread, stack_frame *f, object **exception );

struct scalar_string {
    uint8_t *string;
    size_t   size;
    size_t   refs;
};

struct dict_meta {
    uint64_t  seed;
    instance *instance;
    uint8_t   use_fnv;
};

struct io {
    FILE *fp;
};

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

typedef enum {
    SET_FROM_INT,
    SET_FROM_DEC,
    SET_FROM_STRL,
    SET_FROM_CSTR,
    SET_FROM_OBJ
} scalar_init;

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

struct object {
    object  *type;
    void    *data;
    uint64_t hash;

    size_t refcount;
    object *gc_next;
};

struct subop {
    object *item;
    enum {
        OP_END = 0, // END OF OPS
        OP_OPOP,  // Grab object from arg list
        OP_OPUSH, // Push object to arg list
        OP_SPUSH, // Push symbol to arg list
        OP_APUSH, // Push new arg list
        OP_OCALL, // Pops arg list and runs with it
        OP_SCALL, // Pops arg list and runs with it

        OP_OINST, // Set the instance object
        OP_SINST,
    } op;
};

struct subroutine {
    dict *symbols;
    dict *closures;
    subop *entry;
};

struct instance {
    dict *symbol_table;
    object *main_thread;

    object *io_t;
    object *type_t;
    object *graph_t;
    object *object_t;
    object *scalar_t;
    object *thread_t;
    object *cfunction_t;
    object *subroutine_t;
    object *stack_frame_t;
};

struct arg_list {
    size_t size;
    size_t length;
    size_t index;
    object  *inst;
    object **list;
    arg_list *parent;
};

struct stack_frame {
    stack_frame *parent;
    arg_list *args;
    dict   *symbols;
    object *call;
    union {
        subop *subop;
        int    complete;
    } current;
};

struct thread {
    pthread_t pthread;
    instance *instance;

    arg_list *arg_stack;
    stack_frame *sub_stack;

    object *alloc;
};

extern dict_methods DMETH;
extern uint64_t HASH_SEED;

int    obj_compare( void *meta, void *obj1, void *obj2 );
size_t obj_locate( size_t slot_count, void *meta, void *obj );
void   obj_ref( dict *d, void *obj, int delta );

uint64_t hash_bytes( uint8_t *data, size_t length, uint64_t seed );

object *spawn( object *parent, object *run );

void *spawn_worker( void * );

dict *init_symbol_table();

object *alloc_object( object *t, object *type_o, void *data );

object *init_type( object *t, object *type, object *parent, int refcount );

instance *init_instance();

object *create_scalar( object *t, scalar_init vt, ... );

scalar_string *build_string( uint8_t *raw, size_t bytes );

int insert_symbol( instance *i, const char *name, object *type_o );

void cleanup( instance *i );

int init_io( instance *i );

object *call_non_sub( object *to, stack_frame *sf );

object *io_call( object *thread, stack_frame *sf, object **exception );

int64_t obj_int_val( object *t, object *o );
double  obj_dec_val( object *t, object *o );
scalar_string *obj_str_val( object *t, object *o );
