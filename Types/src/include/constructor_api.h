#ifndef GSD_GC_CONSTRUCTOR_API_H
#define GSD_GC_CONSTRUCTOR_API_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "../GSD_Dictionary/src/include/gsd_dict.h"
#include "exceptions_api.h"
#include "collector_api.h"

typedef struct object object;

typedef struct attr_data attr_data;

typedef enum { A_READ, A_WRITE, A_RW, A_CLEAR, A_CHECK } access_type;

struct attr_data {
    // String -> String dictionary
    dict *delegates;

    // Each must be a string
    object *reader;
    object *writer;
    object *clear;
    object *check;

    // Application specific requirements
    // GSD will require something runnable for each.
    object *builder;
    object *transmute;

    uint8_t required;
};

return_set new_typed_ptr ( object   *type,  void *ptr );
return_set new_type      ( object   *parent           );
return_set new_typed     ( object   *type             );
object    *new_pointer   ( void     *data             );
object    *new_int       ( int64_t   data             );
object    *new_dec       ( double    data             );
object    *new_chars     ( uint8_t  *data             );
object    *new_handle    ( FILE     *data             );
object    *new_dict      ( dict     *data             );
object    *new_string    ( uint8_t  *data             );

object *new_attribute_simple ( uint8_t *name, uint8_t required, access_type type, int types, ... );
object *new_attribute_complex( uint8_t *name, uint8_t required, attr_data   data, int types, ... );

// Null terminated list of string objects
object *new_rope ( object *s, ... );

#endif
