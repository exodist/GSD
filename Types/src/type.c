#include "include/type_api.h"
#include "include/static_api.h"
#include "type.h"
#include "structures.h"
#include "collector.h"
#include <assert.h>

int is_primitive( object *o ) {
    return o->primitive ? 1 : 0;
}

int is_string( object *o ) {
    return o->primitive >= STRING_TYPE_START;
}

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

//return_set type_get_attr( object *t, object *name ) {
//    assert( t->ref_count );
//    return_set r = { .e = EX_NONE, .r = NULL };
//
//    // ensure t is a type-object
//    return_set isa = object_isa_type(t, gc_get_type(GC_TYPE));
//    if ( isa.e ) return isa;
//    if ( isa.r == O_FALSE ) {
//        r.e = EX_NOT_A_TYPE;
//        return r;
//    }
//
//    // return NULL if storage type is not dict
//    if (type_get_store(t) != TS_DICT) {
//        r.e = EX_NO_ATTRIBUTES;
//        return r;
//    };
//
//    // get attr from attr dict
//
//    if (r.r) gc_add_ref(r.r);
//
//    return r;
//}
