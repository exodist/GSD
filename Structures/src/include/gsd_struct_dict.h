#ifndef GSD_STRUCT_DICT_H
#define GSD_STRUCT_DICT_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"

typedef struct dict_iterator dict_iterator;

dict *dict_create(singularity *s, size_t slots, size_t imbalance);

dict *dict_clone(dict *d, int ref, pool *threads);
dict *dict_clone_immutable(dict *d, pool *threads);
dict *make_immutable(dict *d, pool *threads);

result dict_merge(dict *dest, dict *origin, merge_method m);

result dict_free(dict *d);

result dict_get   (dict *d, object *key); // Retrieve item
result dict_share (dict *d, object *key); // Remove shared value, keep shared
result dict_delete(dict *d, object *key); // Remove item without modifying shared values
result dict_deref (dict *d, object *key); // Share the item (get a reference)
result dict_unlink(dict *d, object *key); // Unshare the item, keep locally

result dict_set   (dict *d, object *key, object *val);
result dict_insert(dict *d, object *key, object *val);
result dict_update(dict *d, object *key, object *val);

result dict_set_ref   (dict *d, object *key, ref *val);
result dict_insert_ref(dict *d, object *key, ref *val);
result dict_update_ref(dict *d, object *key, ref *val);

result dict_val_cmp_swap       (dict *a, object *key, object *old, object *new);
result dict_ref_cmp_swap       (dict *a, object *key, ref    *old, ref    *new);
result dict_val_to_ref_cmp_swap(dict *a, object *key, object *old, ref    *new);
result dict_ref_to_val_cmp_swap(dict *a, object *key, ref    *old, object *new);

result dict_cmp_val_delete(dict *a, object *key, object *old);
result dict_cmp_ref_delete(dict *a, object *key, ref    *old);
result dict_cmp_val_deref (dict *a, object *key, object *old);
result dict_cmp_ref_deref (dict *a, object *key, ref    *old);

result dict_fetch_set(dict *a, object *key, object *val);
result dict_fetch_set_ref(dict *a, object *key, ref *val);

// Iteration
dict_iterator *dict_iterate(dict *d);
result         dict_iterate_next(dict_iterator *i);
void           dict_iterator_free(dict_iterator *i);

#endif
