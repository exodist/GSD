#ifndef GSD_GC_OBJECT_API_H
#define GSD_GC_OBJECT_API_H

#include <stdio.h>
#include <stdint.h>
#include "../GSD_Dictionary/src/include/gsd_dict.h"
#include "exceptions_api.h"

typedef struct object object;

// These will assert that you passed in the right value.
dict    *object_get_store   ( object *o );
void    *object_get_pointer ( object *o );
int64_t  object_get_sint    ( object *o );
double   object_get_dec     ( object *o );
FILE    *object_get_handle  ( object *o );
dict    *object_get_dict    ( object *o );
uint8_t *object_get_string  ( object *o );

object *object_get_type ( object *o );

return_set  object_isa_type ( object *o, object *type );
return_set  object_does     ( object *o, object *role );
object     *object_can      ( object *o, object *name );
object     *object_access   ( object *o, object *name, object *arg );

#endif
