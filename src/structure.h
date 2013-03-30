#ifndef GSD_STRUCTURE_H
#define GSD_STRUCTURE_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

typedef struct object    object;
typedef struct instance  instance;
typedef struct dict_meta dict_meta;
typedef struct scalar_string scalar_string;

struct object {
    object  *type;
    void    *data;
    uint64_t hash;

    size_t refcount;
    object *gc_next;
};

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

typedef enum {
    SET_FROM_INT,
    SET_FROM_DEC,
    SET_FROM_STRL,
    SET_FROM_STRL_NUM,
    SET_FROM_CSTR,
    SET_FROM_OBJ
} scalar_init;

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
    object *keyword_t;
};

extern dict_methods DMETH;
extern dict_methods OMETH;
extern uint64_t HASH_SEED;

int    obj_ocompare( void *meta, void *obj1, void *obj2 );
size_t obj_olocate( size_t slot_count, void *meta, void *obj );

int    obj_compare( void *meta, void *obj1, void *obj2 );
size_t obj_locate( size_t slot_count, void *meta, void *obj );

void  obj_ref( dict *d, void *obj, int delta );
char *dot_decode( void *key, void *val );

uint64_t hash_bytes( uint8_t *data, size_t length, uint64_t seed );

int64_t obj_int_val( object *t, object *o );
double  obj_dec_val( object *t, object *o );
scalar_string *obj_str_val( object *t, object *o );

scalar_string *build_string( uint8_t *raw, size_t bytes );

#endif
