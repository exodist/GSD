#include "include/constructor_api.h"
#include "constructor.h"
#include "structures.h"
#include "static.h"
#include <assert.h>

return_set new_typed_ptr ( object *type, void *ptr );
return_set new_type      ( object *parent );
return_set new_typed     ( object *type   );

object *new_pointer ( void    *data );
object *new_dec     ( double   data );
object *new_handle  ( FILE    *data );
object *new_dict    ( dict    *data );

object *new_chars   ( uint8_t *data  );
object *new_string  ( uint8_t *data  );
object *new_rope    ( object *s, ... );

object *new_int( int64_t data ) {
    if (!data)                   return O_ZERO;
    if (data > 0 && data <  127) return &(O_PINTS[data]);
    if (data < 0 && data > -127) return &(O_NINTS[data * -1]);
    assert(0);
}
