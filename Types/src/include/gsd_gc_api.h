#ifndef GC_API_H
#define GC_API_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "../GSD_Dictionary/src/include/gsd_dict.h"

typedef struct object     object;
typedef struct collector  collector;
typedef struct collection collection;

extern object *O_TRUE;
extern object *O_FALSE;
extern object *O_UNDEF;
extern object *O_ZERO;
extern object *O_PINTS; // has 1 ->  127
extern object *O_NINTS; // has 1 -> -127

typedef enum {
    EX_NONE = 0,
    EX_OUT_OF_MEMORY,
    EX_NO_ATTRIBUTES,
    EX_NOT_A_TYPE,
} exception;

typedef enum {
    GC_TYPED   = 0,
    GC_TYPE    = 1,
    GC_POINTER = 2,
    GC_INT     = 3,
    GC_DEC     = 4,
    GC_SNIP    = 5,
    GC_HANDLE  = 6,
    GC_DICT    = 7,
    GC_STRING  = 8,
    GC_ROPE    = 9,
    GC_BOOL    = 10,
    GC_UNDEF   = 11,
    GC_FREE
} primitive;

typedef enum { TS_DICT, TS_POINTER } type_storage;

typedef struct {
    exception e;
    object   *r;
} return_set;

collector *collector_spawn (               );
void       collector_pause ( collector *cr );
void       collector_free  ( collector *cr );

collection *gc_get_collection ( collector *cr                 );
void        gc_ret_collection ( collector *cr, collection *cn );

uint32_t gc_add_ref( object *o );
uint32_t gc_del_ref( object *o );

// Get the type-object for the specified primitive type
object *gc_get_type ( primitive p );

return_set new_typed_ptr ( object *type, void *ptr );
return_set new_type      ( object *parent );
return_set new_typed     ( object *type   );

object *new_pointer ( void    *data );
object *new_sint    ( int64_t  data );
object *new_dec     ( double   data );
object *new_chars   ( uint8_t *data );
object *new_handle  ( FILE    *data );
object *new_dict    ( dict    *data );
object *new_string  ( uint8_t *data );

// Null terminated string of objects
object *new_rope ( collection *c, ... );

// These will assert that you passed in the right value.
dict    *object_get_store   ( object *o );
void    *object_get_pointer ( object *o );
int64_t  object_get_sint    ( object *o );
double   object_get_dec     ( object *o );
uint8_t *object_get_chars   ( object *o );
FILE    *object_get_handle  ( object *o );
dict    *object_get_dict    ( object *o );
uint8_t *object_get_string  ( object *o );

object *object_get_type ( object *o );

return_set  object_isa_type ( object *o, object *type              );
return_set  object_does     ( object *o, object *role              );
object     *object_can      ( object *o, object *name              );
return_set  object_has_attr ( object *o, object *name              );
return_set  object_get_attr ( object *o, object *name              );
exception   object_set_attr ( object *o, object *name, object *val );

exception    type_compile    ( object *t                             );
type_storage type_get_store  ( object *t                             );
exception    type_set_store  ( object *t, type_storage ts            );
return_set   type_is_mutable ( object *t                             );
return_set   type_isa_type   ( object *t, object *parent             );
return_set   type_does       ( object *t, object *role               );
exception    type_add_role   ( object *t, object *role               );
exception    type_add_symbol ( object *t, object *name, object *meth );
return_set   type_get_symbol ( object *t, object *name               );
exception    type_add_attr   ( object *t, object *name, object *attr );
return_set   type_get_attr   ( object *t, object *name               );

#endif
