#ifndef GSD_STRUCT_REF_H
#define GSD_STRUCT_REF_H

#include "gsd_struct_types.h"

ref *ref_create(void *val, refdelta *d, trigger *t, void *t_arg);

result ref_get(ref *r);
result ref_set(ref *r, void *val);
result ref_update(ref *r, void *val);
result ref_insert(ref *r, void *val);
result ref_delete(ref *r);

void ref_free(ref *r);

#endif

