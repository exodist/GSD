#ifndef GSD_STRUCT_TREE_H
#define GSD_STRUCT_TREE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

typedef struct tree_iterator tree_iterator;

tree *tree_create(alloc *refs);
tree *tree_clone(tree *t, int ref);

result *tree_merge(tree *dest, tree *from, merge_method m);

result tree_make_immutable(tree *t);

result tree_get   (tree *t, object *key); // Retrieve item
result tree_delete(tree *t, object *key); // Remove shared value, keep shared
result tree_deref (tree *t, object *key); // Remove item without modifying shared values
result tree_share (tree *t, object *key); // Share the item (get a reference)
result tree_unlink(tree *t, object *key); // Unshare the item, keep locally

// Transactions
result tree_set   (tree *t, object *key, object *value);
result tree_insert(tree *t, object *key, object *value);
result tree_update(tree *t, object *key, object *value);

// Transactions with shared items
result tree_set_ref   (tree *t, object *key, ref *value);
result tree_insert_ref(tree *t, object *key, ref *value);
result tree_update_ref(tree *t, object *key, ref *value);

// Atomic compare+swaps
result tree_val_cmp_swap       (tree *t, object *key, object *old, object *new);
result tree_ref_cmp_swap       (tree *t, object *key, ref    *old, ref    *new);
result tree_val_to_ref_cmp_swap(tree *t, object *key, object *old, ref    *new);
result tree_ref_to_val_cmp_swap(tree *t, object *key, ref    *old, object *new);

result tree_cmp_val_delete(dict *a, object *key, object *old);
result tree_cmp_ref_delete(dict *a, object *key, ref    *old);
result tree_cmp_val_deref (dict *a, object *key, object *old);
result tree_cmp_ref_deref (dict *a, object *key, ref    *old);

result tree_fetch_set(dict *a, object *key, object *val);
result tree_fetch_set_ref(dict *a, object *key, ref *val);

// Iteration
tree_iterator *tree_iterate(tree *t);
result         tree_iterate_next(tree_iterator *i);
void           tree_iterator_free(tree_iterator *i);

#endif
