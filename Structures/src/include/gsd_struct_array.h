#ifndef GSD_STRUCT_ARRAY_H
#define GSD_STRUCT_ARRAY_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"

typedef struct array_iterator array_iterator;

array *array_create(int64_t init_size, int64_t grow_delta);

result array_get   (array *a, int64_t idx); // Retrieve item
result array_share (array *a, int64_t idx); // Remove shared value, keep shared
result array_delete(array *a, int64_t idx); // Remove item without modifying shared values
result array_deref (array *a, int64_t idx); // Share the item (get a reference)
result array_unlink(array *a, int64_t idx); // Unshare the item, keep locally

result array_set       (array *a, int64_t idx, object *val);
result array_update    (array *a, int64_t idx, object *val);
result array_insert    (array *a, int64_t idx, object *val);
result array_set_ref   (array *a, int64_t idx, ref    *val);
result array_update_ref(array *a, int64_t idx, ref    *val);
result array_insert_ref(array *a, int64_t idx, ref    *val);

result array_pop  (array *a);
result array_shift(array *a);

int64_t array_push       (array *a, object *val);
int64_t array_unshift    (array *a, object *val);
int64_t array_push_ref   (array *a, ref  *val);
int64_t array_unshift_ref(array *a, ref  *val);

result array_val_cmp_swap       (array *a, int64_t idx, object *old, object *new);
result array_ref_cmp_swap       (array *a, int64_t idx, ref    *old, ref    *new);
result array_val_to_ref_cmp_swap(array *a, int64_t idx, object *old, ref    *new);
result array_ref_to_val_cmp_swap(array *a, int64_t idx, ref    *old, object *new);

result array_cmp_val_delete(array *a, int64_t idx, object *old);
result array_cmp_ref_delete(array *a, int64_t idx, ref    *old);
result array_cmp_val_deref (array *a, int64_t idx, object *old);
result array_cmp_ref_deref (array *a, int64_t idx, ref    *old);

// Iteration
array_iterator *array_iterate(array *a);
result          array_iterate_next(array_iterator *i);
void            array_iterator_free(array_iterator *i);

#endif
