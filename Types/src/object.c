#include "include/object_api.h"
#include "object.h"
#include "structures.h"

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
