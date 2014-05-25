#ifndef GSD_STRUCT_REF_H
#define GSD_STRUCT_REF_H

#include "gsd_struct_types.h"

result ref_create(singularity *s, object *val);

result ref_get(ref *r);

result ref_set   (ref *r, object *val);
result ref_update(ref *r, object *val);
result ref_insert(ref *r, object *val);

void ref_free(ref *r);

#endif

