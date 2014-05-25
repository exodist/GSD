#ifndef GSD_STRUCT_OBJECT_H
#define GSD_STRUCT_OBJECT_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

object *object_create(singularity *s, object *class, ...);

object *object_build_cstring(singularity *s, const char *val);
object *object_build_cnumber(singularity *s, int64_t val);

object *object_class(object *o);

result object_resolve(object *o, object *key); // For methods and attributes in class

result object_get(object *o, object *key);

result object_set   (object *o, object *key, object *val);
result object_insert(object *o, object *key, object *val);
result object_update(object *o, object *key, object *val);
result object_delete(object *o, object *key, object *val);

result object_cmp_delete  (object *o, object *key, object *val);
result object_fetch_delete(object *o, object *key, object *val);

result object_cmp_update  (object *o, object *key, object *old, object *new);
result object_fetch_update(object *o, object *key, object *old, object *new);

#endif
