#include "include/gsd_gc_api.h"
#include "structures.h"
#include <assert.h>

#define OS_INT_T   {.type = GC_INT,   .state = GC_UNCHECKED, .permanent = 1, .epoch = 0, .ref_count = 1}
#define OS_BOOL_T  {.type = GC_BOOL,  .state = GC_UNCHECKED, .permanent = 1, .epoch = 0, .ref_count = 1}
#define OS_UNDEF_T {.type = GC_UNDEF, .state = GC_UNCHECKED, .permanent = 1, .epoch = 0, .ref_count = 1}

object_simple OS_TRUE  = { .object = OS_BOOL_T,  .simple_data.integer = 1 };
object_simple OS_FALSE = { .object = OS_BOOL_T,  .simple_data.integer = 0 };
object_simple OS_UNDEF = { .object = OS_UNDEF_T, .simple_data.integer = 0 };
object_simple OS_ZERO  = { .object = OS_INT_T,   .simple_data.integer = 0 };

object_simple OS_PINTS[128] = {
    {.object = OS_INT_T, .simple_data.integer = 0  },
    {.object = OS_INT_T, .simple_data.integer = 1  }, {.object = OS_INT_T, .simple_data.integer = 2  },
    {.object = OS_INT_T, .simple_data.integer = 3  }, {.object = OS_INT_T, .simple_data.integer = 4  },
    {.object = OS_INT_T, .simple_data.integer = 5  }, {.object = OS_INT_T, .simple_data.integer = 6  },
    {.object = OS_INT_T, .simple_data.integer = 7  }, {.object = OS_INT_T, .simple_data.integer = 8  },
    {.object = OS_INT_T, .simple_data.integer = 9  }, {.object = OS_INT_T, .simple_data.integer = 10 },
    {.object = OS_INT_T, .simple_data.integer = 11 }, {.object = OS_INT_T, .simple_data.integer = 12 },
    {.object = OS_INT_T, .simple_data.integer = 13 }, {.object = OS_INT_T, .simple_data.integer = 14 },
    {.object = OS_INT_T, .simple_data.integer = 15 }, {.object = OS_INT_T, .simple_data.integer = 16 },
    {.object = OS_INT_T, .simple_data.integer = 17 }, {.object = OS_INT_T, .simple_data.integer = 18 },
    {.object = OS_INT_T, .simple_data.integer = 19 }, {.object = OS_INT_T, .simple_data.integer = 20 },
    {.object = OS_INT_T, .simple_data.integer = 21 }, {.object = OS_INT_T, .simple_data.integer = 22 },
    {.object = OS_INT_T, .simple_data.integer = 23 }, {.object = OS_INT_T, .simple_data.integer = 24 },
    {.object = OS_INT_T, .simple_data.integer = 25 }, {.object = OS_INT_T, .simple_data.integer = 26 },
    {.object = OS_INT_T, .simple_data.integer = 27 }, {.object = OS_INT_T, .simple_data.integer = 28 },
    {.object = OS_INT_T, .simple_data.integer = 29 }, {.object = OS_INT_T, .simple_data.integer = 30 },
    {.object = OS_INT_T, .simple_data.integer = 31 }, {.object = OS_INT_T, .simple_data.integer = 32 },
    {.object = OS_INT_T, .simple_data.integer = 33 }, {.object = OS_INT_T, .simple_data.integer = 34 },
    {.object = OS_INT_T, .simple_data.integer = 35 }, {.object = OS_INT_T, .simple_data.integer = 36 },
    {.object = OS_INT_T, .simple_data.integer = 37 }, {.object = OS_INT_T, .simple_data.integer = 38 },
    {.object = OS_INT_T, .simple_data.integer = 39 }, {.object = OS_INT_T, .simple_data.integer = 40 },
    {.object = OS_INT_T, .simple_data.integer = 41 }, {.object = OS_INT_T, .simple_data.integer = 42 },
    {.object = OS_INT_T, .simple_data.integer = 43 }, {.object = OS_INT_T, .simple_data.integer = 44 },
    {.object = OS_INT_T, .simple_data.integer = 45 }, {.object = OS_INT_T, .simple_data.integer = 46 },
    {.object = OS_INT_T, .simple_data.integer = 47 }, {.object = OS_INT_T, .simple_data.integer = 48 },
    {.object = OS_INT_T, .simple_data.integer = 49 }, {.object = OS_INT_T, .simple_data.integer = 50 },
    {.object = OS_INT_T, .simple_data.integer = 51 }, {.object = OS_INT_T, .simple_data.integer = 52 },
    {.object = OS_INT_T, .simple_data.integer = 53 }, {.object = OS_INT_T, .simple_data.integer = 54 },
    {.object = OS_INT_T, .simple_data.integer = 55 }, {.object = OS_INT_T, .simple_data.integer = 56 },
    {.object = OS_INT_T, .simple_data.integer = 57 }, {.object = OS_INT_T, .simple_data.integer = 58 },
    {.object = OS_INT_T, .simple_data.integer = 59 }, {.object = OS_INT_T, .simple_data.integer = 60 },
    {.object = OS_INT_T, .simple_data.integer = 61 }, {.object = OS_INT_T, .simple_data.integer = 62 },
    {.object = OS_INT_T, .simple_data.integer = 63 }, {.object = OS_INT_T, .simple_data.integer = 64 },
    {.object = OS_INT_T, .simple_data.integer = 65 }, {.object = OS_INT_T, .simple_data.integer = 66 },
    {.object = OS_INT_T, .simple_data.integer = 67 }, {.object = OS_INT_T, .simple_data.integer = 68 },
    {.object = OS_INT_T, .simple_data.integer = 69 }, {.object = OS_INT_T, .simple_data.integer = 70 },
    {.object = OS_INT_T, .simple_data.integer = 71 }, {.object = OS_INT_T, .simple_data.integer = 72 },
    {.object = OS_INT_T, .simple_data.integer = 73 }, {.object = OS_INT_T, .simple_data.integer = 74 },
    {.object = OS_INT_T, .simple_data.integer = 75 }, {.object = OS_INT_T, .simple_data.integer = 76 },
    {.object = OS_INT_T, .simple_data.integer = 77 }, {.object = OS_INT_T, .simple_data.integer = 78 },
    {.object = OS_INT_T, .simple_data.integer = 79 }, {.object = OS_INT_T, .simple_data.integer = 80 },
    {.object = OS_INT_T, .simple_data.integer = 81 }, {.object = OS_INT_T, .simple_data.integer = 82 },
    {.object = OS_INT_T, .simple_data.integer = 83 }, {.object = OS_INT_T, .simple_data.integer = 84 },
    {.object = OS_INT_T, .simple_data.integer = 85 }, {.object = OS_INT_T, .simple_data.integer = 86 },
    {.object = OS_INT_T, .simple_data.integer = 87 }, {.object = OS_INT_T, .simple_data.integer = 88 },
    {.object = OS_INT_T, .simple_data.integer = 89 }, {.object = OS_INT_T, .simple_data.integer = 90 },
    {.object = OS_INT_T, .simple_data.integer = 91 }, {.object = OS_INT_T, .simple_data.integer = 92 },
    {.object = OS_INT_T, .simple_data.integer = 93 }, {.object = OS_INT_T, .simple_data.integer = 94 },
    {.object = OS_INT_T, .simple_data.integer = 95 }, {.object = OS_INT_T, .simple_data.integer = 96 },
    {.object = OS_INT_T, .simple_data.integer = 97 }, {.object = OS_INT_T, .simple_data.integer = 98 },
    {.object = OS_INT_T, .simple_data.integer = 99 }, {.object = OS_INT_T, .simple_data.integer = 100},
    {.object = OS_INT_T, .simple_data.integer = 101}, {.object = OS_INT_T, .simple_data.integer = 102},
    {.object = OS_INT_T, .simple_data.integer = 103}, {.object = OS_INT_T, .simple_data.integer = 104},
    {.object = OS_INT_T, .simple_data.integer = 105}, {.object = OS_INT_T, .simple_data.integer = 106},
    {.object = OS_INT_T, .simple_data.integer = 107}, {.object = OS_INT_T, .simple_data.integer = 108},
    {.object = OS_INT_T, .simple_data.integer = 109}, {.object = OS_INT_T, .simple_data.integer = 110},
    {.object = OS_INT_T, .simple_data.integer = 111}, {.object = OS_INT_T, .simple_data.integer = 112},
    {.object = OS_INT_T, .simple_data.integer = 113}, {.object = OS_INT_T, .simple_data.integer = 114},
    {.object = OS_INT_T, .simple_data.integer = 115}, {.object = OS_INT_T, .simple_data.integer = 116},
    {.object = OS_INT_T, .simple_data.integer = 117}, {.object = OS_INT_T, .simple_data.integer = 118},
    {.object = OS_INT_T, .simple_data.integer = 119}, {.object = OS_INT_T, .simple_data.integer = 120},
    {.object = OS_INT_T, .simple_data.integer = 121}, {.object = OS_INT_T, .simple_data.integer = 122},
    {.object = OS_INT_T, .simple_data.integer = 123}, {.object = OS_INT_T, .simple_data.integer = 124},
    {.object = OS_INT_T, .simple_data.integer = 125}, {.object = OS_INT_T, .simple_data.integer = 126},
    {.object = OS_INT_T, .simple_data.integer = 127}
};

object_simple OS_NINTS[128] = {
    {.object = OS_INT_T, .simple_data.integer =  0  },
    {.object = OS_INT_T, .simple_data.integer = -1  }, {.object = OS_INT_T, .simple_data.integer = -2  },
    {.object = OS_INT_T, .simple_data.integer = -3  }, {.object = OS_INT_T, .simple_data.integer = -4  },
    {.object = OS_INT_T, .simple_data.integer = -5  }, {.object = OS_INT_T, .simple_data.integer = -6  },
    {.object = OS_INT_T, .simple_data.integer = -7  }, {.object = OS_INT_T, .simple_data.integer = -8  },
    {.object = OS_INT_T, .simple_data.integer = -9  }, {.object = OS_INT_T, .simple_data.integer = -10 },
    {.object = OS_INT_T, .simple_data.integer = -11 }, {.object = OS_INT_T, .simple_data.integer = -12 },
    {.object = OS_INT_T, .simple_data.integer = -13 }, {.object = OS_INT_T, .simple_data.integer = -14 },
    {.object = OS_INT_T, .simple_data.integer = -15 }, {.object = OS_INT_T, .simple_data.integer = -16 },
    {.object = OS_INT_T, .simple_data.integer = -17 }, {.object = OS_INT_T, .simple_data.integer = -18 },
    {.object = OS_INT_T, .simple_data.integer = -19 }, {.object = OS_INT_T, .simple_data.integer = -20 },
    {.object = OS_INT_T, .simple_data.integer = -21 }, {.object = OS_INT_T, .simple_data.integer = -22 },
    {.object = OS_INT_T, .simple_data.integer = -23 }, {.object = OS_INT_T, .simple_data.integer = -24 },
    {.object = OS_INT_T, .simple_data.integer = -25 }, {.object = OS_INT_T, .simple_data.integer = -26 },
    {.object = OS_INT_T, .simple_data.integer = -27 }, {.object = OS_INT_T, .simple_data.integer = -28 },
    {.object = OS_INT_T, .simple_data.integer = -29 }, {.object = OS_INT_T, .simple_data.integer = -30 },
    {.object = OS_INT_T, .simple_data.integer = -31 }, {.object = OS_INT_T, .simple_data.integer = -32 },
    {.object = OS_INT_T, .simple_data.integer = -33 }, {.object = OS_INT_T, .simple_data.integer = -34 },
    {.object = OS_INT_T, .simple_data.integer = -35 }, {.object = OS_INT_T, .simple_data.integer = -36 },
    {.object = OS_INT_T, .simple_data.integer = -37 }, {.object = OS_INT_T, .simple_data.integer = -38 },
    {.object = OS_INT_T, .simple_data.integer = -39 }, {.object = OS_INT_T, .simple_data.integer = -40 },
    {.object = OS_INT_T, .simple_data.integer = -41 }, {.object = OS_INT_T, .simple_data.integer = -42 },
    {.object = OS_INT_T, .simple_data.integer = -43 }, {.object = OS_INT_T, .simple_data.integer = -44 },
    {.object = OS_INT_T, .simple_data.integer = -45 }, {.object = OS_INT_T, .simple_data.integer = -46 },
    {.object = OS_INT_T, .simple_data.integer = -47 }, {.object = OS_INT_T, .simple_data.integer = -48 },
    {.object = OS_INT_T, .simple_data.integer = -49 }, {.object = OS_INT_T, .simple_data.integer = -50 },
    {.object = OS_INT_T, .simple_data.integer = -51 }, {.object = OS_INT_T, .simple_data.integer = -52 },
    {.object = OS_INT_T, .simple_data.integer = -53 }, {.object = OS_INT_T, .simple_data.integer = -54 },
    {.object = OS_INT_T, .simple_data.integer = -55 }, {.object = OS_INT_T, .simple_data.integer = -56 },
    {.object = OS_INT_T, .simple_data.integer = -57 }, {.object = OS_INT_T, .simple_data.integer = -58 },
    {.object = OS_INT_T, .simple_data.integer = -59 }, {.object = OS_INT_T, .simple_data.integer = -60 },
    {.object = OS_INT_T, .simple_data.integer = -61 }, {.object = OS_INT_T, .simple_data.integer = -62 },
    {.object = OS_INT_T, .simple_data.integer = -63 }, {.object = OS_INT_T, .simple_data.integer = -64 },
    {.object = OS_INT_T, .simple_data.integer = -65 }, {.object = OS_INT_T, .simple_data.integer = -66 },
    {.object = OS_INT_T, .simple_data.integer = -67 }, {.object = OS_INT_T, .simple_data.integer = -68 },
    {.object = OS_INT_T, .simple_data.integer = -69 }, {.object = OS_INT_T, .simple_data.integer = -70 },
    {.object = OS_INT_T, .simple_data.integer = -71 }, {.object = OS_INT_T, .simple_data.integer = -72 },
    {.object = OS_INT_T, .simple_data.integer = -73 }, {.object = OS_INT_T, .simple_data.integer = -74 },
    {.object = OS_INT_T, .simple_data.integer = -75 }, {.object = OS_INT_T, .simple_data.integer = -76 },
    {.object = OS_INT_T, .simple_data.integer = -77 }, {.object = OS_INT_T, .simple_data.integer = -78 },
    {.object = OS_INT_T, .simple_data.integer = -79 }, {.object = OS_INT_T, .simple_data.integer = -80 },
    {.object = OS_INT_T, .simple_data.integer = -81 }, {.object = OS_INT_T, .simple_data.integer = -82 },
    {.object = OS_INT_T, .simple_data.integer = -83 }, {.object = OS_INT_T, .simple_data.integer = -84 },
    {.object = OS_INT_T, .simple_data.integer = -85 }, {.object = OS_INT_T, .simple_data.integer = -86 },
    {.object = OS_INT_T, .simple_data.integer = -87 }, {.object = OS_INT_T, .simple_data.integer = -88 },
    {.object = OS_INT_T, .simple_data.integer = -89 }, {.object = OS_INT_T, .simple_data.integer = -90 },
    {.object = OS_INT_T, .simple_data.integer = -91 }, {.object = OS_INT_T, .simple_data.integer = -92 },
    {.object = OS_INT_T, .simple_data.integer = -93 }, {.object = OS_INT_T, .simple_data.integer = -94 },
    {.object = OS_INT_T, .simple_data.integer = -95 }, {.object = OS_INT_T, .simple_data.integer = -96 },
    {.object = OS_INT_T, .simple_data.integer = -97 }, {.object = OS_INT_T, .simple_data.integer = -98 },
    {.object = OS_INT_T, .simple_data.integer = -99 }, {.object = OS_INT_T, .simple_data.integer = -100},
    {.object = OS_INT_T, .simple_data.integer = -101}, {.object = OS_INT_T, .simple_data.integer = -102},
    {.object = OS_INT_T, .simple_data.integer = -103}, {.object = OS_INT_T, .simple_data.integer = -104},
    {.object = OS_INT_T, .simple_data.integer = -105}, {.object = OS_INT_T, .simple_data.integer = -106},
    {.object = OS_INT_T, .simple_data.integer = -107}, {.object = OS_INT_T, .simple_data.integer = -108},
    {.object = OS_INT_T, .simple_data.integer = -109}, {.object = OS_INT_T, .simple_data.integer = -110},
    {.object = OS_INT_T, .simple_data.integer = -111}, {.object = OS_INT_T, .simple_data.integer = -112},
    {.object = OS_INT_T, .simple_data.integer = -113}, {.object = OS_INT_T, .simple_data.integer = -114},
    {.object = OS_INT_T, .simple_data.integer = -115}, {.object = OS_INT_T, .simple_data.integer = -116},
    {.object = OS_INT_T, .simple_data.integer = -117}, {.object = OS_INT_T, .simple_data.integer = -118},
    {.object = OS_INT_T, .simple_data.integer = -119}, {.object = OS_INT_T, .simple_data.integer = -120},
    {.object = OS_INT_T, .simple_data.integer = -121}, {.object = OS_INT_T, .simple_data.integer = -122},
    {.object = OS_INT_T, .simple_data.integer = -123}, {.object = OS_INT_T, .simple_data.integer = -124},
    {.object = OS_INT_T, .simple_data.integer = -125}, {.object = OS_INT_T, .simple_data.integer = -126},
    {.object = OS_INT_T, .simple_data.integer = -127}
};

object *O_TRUE  = (object *)&OS_TRUE;
object *O_FALSE = (object *)&OS_FALSE;
object *O_UNDEF = (object *)&OS_UNDEF;
object *O_ZERO  = (object *)&OS_ZERO;
object *O_PINTS = (object *)OS_PINTS;
object *O_NINTS = (object *)OS_NINTS;

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
object *new_uint    ( uint64_t data );
object *new_dec     ( double   data );
object *new_chars   ( uint8_t *data );
object *new_handle  ( FILE    *data );
object *new_dict    ( dict    *data );
object *new_string  ( uint8_t *data );

// Null terminated string of objects and/or c-strings
object *new_rope ( collection *c, ... );

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

return_set type_get_attr( object *t, object *name ) {
    assert( t->ref_count );
    return_set r = { .e = EX_NONE, .r = NULL };

    // ensure t is a type-object
    return_set isa = object_isa_type(t, gc_get_type(GC_TYPE));
    if ( isa.e ) return isa;
    if ( isa.r == O_FALSE ) {
        r.e = EX_NOT_A_TYPE;
        return r;
    }

    // return NULL if storage type is not dict
    if (type_get_store(t) != TS_DICT) {
        r.e = EX_NO_ATTRIBUTES;
        return r;
    };

    // get attr from attr dict

    if (r.r) gc_add_ref(r.r);

    return r;
}
