#ifndef GSD_STRUCT_DICT_H
#define GSD_STRUCT_DICT_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"

typedef int   (dict_cmp) (void *meta, void *key1, void  *key2);
typedef size_t(dict_loc) (void *meta, void *key,  size_t slot_count);
typedef void(dict_change)(void *meta, void *key,  void  *old_val, void *new_val);

typedef int(dict_handler)(void *key, void *val, void *args);

typedef enum {MERGE_SET, MERGE_INSERT, MERGE_UPDATE} dict_merge_op;

typedef struct dict_builder dict_builder;
struct dict_builder {
    dict_cmp    *cmp;
    dict_loc    *loc;
    refdelta    *dlt;
    dict_change *cng;
};

// Constructors
dict *dict_create(void *meta, dict_cmp *cmp, dict_loc *loc, size_t slots, size_t imbalance);
dict *dict_build (void *meta, const dict_builder *b, size_t slots, size_t imbalance);

dict *dict_clone          (void *meta, dict *d, int reference, size_t threads);
dict *dict_clone_immutable(void *meta, dict *d, size_t threads);

dict *dict_build_clone          (void *meta, dict *d, const dict_builder *b, int reference, size_t threads);
dict *dict_build_clone_immutable(void *meta, dict *d, const dict_builder *b, size_t threads);

// Modifiers
result *dict_reset(dict *d, size_t slots, size_t max_imbalance, void *meta);

// Combining
result dict_merge(dict *orig, dict *dest, dict_merge_op op, int reference, size_t threads);

// Cleanup
result dict_rebalance(dict *d, size_t threads);
result dict_free(dict *d);

// Meta/settings/etc
void               *dict_meta     (dict *d);
size_t              dict_slots    (dict *d);
size_t              dict_imbalance(dict *d);
refdelta           *dict_refdelta (dict *d);
const dict_builder *dict_built    (dict *d);

// Utility
int dict_iterate (dict *d, dict_handler *h, void *args);

// Operations

/* get for a given key */
result dict_get    (dict *d, void *key); // Get the value for a given key
result dict_get_ref(dict *d, void *key); // Get the ref   for a given key

/* insert for a given key, fails if it exists already */
result dict_insert    (dict *d, void *key, void      *val); // Insert a new key+value Fail if the key exists
result dict_insert_ref(dict *d, void *key, reference *ref); // Insert a new key+ref   Fail if the key exists

/* set for a given key, it may or may not exist already */
result dict_set          (dict *d, void *key, void      *val); // Set the value for the given key
result dict_set_ref      (dict *d, void *key, reference *ref); // Set the ref   for the given key
result dict_fetch_set    (dict *d, void *key, void      *val); // Set the value and return the previous one
result dict_fetch_set_ref(dict *d, void *key, reference *ref); // Set the ref   and return the previous one

/* update for a given key, fails if it does not exist */
result dict_update          (dict *d, void *key, void      *val); // Update the value for the given key
result dict_update_ref      (dict *d, void *key, reference *ref); // Update the ref   for the given key
result dict_fetch_update    (dict *d, void *key, void      *val); // Set the value and return the previous one
result dict_fetch_update_ref(dict *d, void *key, reference *val); // Set the ref   and return the previous one
result dict_cmp_update      (dict *d, void *key, void      *old_val, void      *new_val); // Atomic swap
result dict_cmp_update_ref  (dict *d, void *key, reference *old_ref, reference *new_ref); // Atomic swap

/* clear the value for a given key (the reference remains in the dict) */
result dict_clear      (dict *d, void *key); // Clear the value for the ref at the given key
result dict_fetch_clear(dict *d, void *key); // Clear the value at the given key, and return it.
result dict_cmp_clear  (dict *d, void *key, void *old_val); // Atomic

/* remove the ref for a given key */
result dict_deref      (dict *d, void *key); // Remove the ref for a given key
result dict_fetch_deref(dict *d, void *key); // Remove the ref at the given key, and return it.
result dict_cmp_deref  (dict *d, void *key, reference *old_ref); // Atomic

#endif
