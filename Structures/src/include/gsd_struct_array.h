#ifndef GSD_STRUCT_ARRAY_H
#define GSD_STRUCT_ARRAY_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"

array *array_create(size_t init_size, size_t grow_delta, refdelta *rd);

result array_get    (array *a, int64_t idx);
result array_get_ref(array *a, int64_t idx);

result array_set       (array *a, int64_t idx, void *val);
result array_update    (array *a, int64_t idx, void *val);
result array_insert    (array *a, int64_t idx, void *val);
result array_set_ref   (array *a, int64_t idx, ref *val);
result array_update_ref(array *a, int64_t idx, ref *val);
result array_insert_ref(array *a, int64_t idx, ref *val);

result array_pop      (array *a);
result array_shift    (array *a);
result array_pop_ref  (array *a);
result array_shift_ref(array *a);

int64_t array_push       (array *a, void *val);
int64_t array_unshift    (array *a, void *val);
int64_t array_push_ref   (array *a, ref *val);
int64_t array_unshift_ref(array *a, ref *val);

result array_cmp_update    (array *a, int64_t idx, void *val);
result array_cmp_update_ref(array *a, int64_t idx, ref *val);

#endif
