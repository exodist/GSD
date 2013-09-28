#include "include/constructor_api.h"
#include "constructor.h"
#include "structures.h"

return_set new_typed_ptr ( object *type, void *ptr );
return_set new_type      ( object *parent );
return_set new_typed     ( object *type   );

object *new_pointer ( void    *data );
object *new_sint    ( int64_t  data );
object *new_uint    ( uint64_t data );
object *new_dec     ( double   data );
object *new_handle  ( FILE    *data );
object *new_dict    ( dict    *data );

object *new_chars   ( uint8_t *data  );
object *new_string  ( uint8_t *data  );
object *new_rope    ( object *s, ... );


