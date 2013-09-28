#ifndef GSD_GC_TYPE_API_H
#define GSD_GC_TYPE_API_H

#include "exceptions_api.h"

typedef struct object object;

typedef enum { TS_DICT, TS_POINTER } type_storage;

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
