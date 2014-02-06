#ifndef GSD_STRUCT_REF_H
#define GSD_STRUCT_REF_H

#include "gsd_struct_types.h"

reference *ref_create(void *val, refdelta *d, dict_trigger *t, void *t_arg);

result ref_set_trigger(reference *r, trigger *t, void *t_arg);
result ref_del_trigger(reference *r);

result ref_get_value(reference *r);
result ref_set_value(reference *r, void *val);
result ref_update_value(reference *r, void *val);
result ref_insert_value(reference *r, void *val);

void ref_free(reference *r);

#endif
