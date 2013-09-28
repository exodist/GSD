#ifndef GSD_GC_CONSTRUCTOR_API_H
#define GSD_GC_CONSTRUCTOR_API_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "../GSD_Dictionary/src/include/gsd_dict.h"
#include "exceptions_api.h"
#include "collector_api.h"

typedef struct object object;

return_set new_typed_ptr ( object  *type,  void *ptr );
return_set new_type      ( object  *parent           );
return_set new_typed     ( object  *type             );
object    *new_pointer   ( void    *data             );
object    *new_sint      ( int64_t  data             );
object    *new_dec       ( double   data             );
object    *new_chars     ( uint8_t *data             );
object    *new_handle    ( FILE    *data             );
object    *new_dict      ( dict    *data             );
object    *new_string    ( uint8_t *data             );

// Null terminated list of string objects
object *new_rope ( object *s, ... );

#endif
