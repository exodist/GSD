#ifndef GSD_STRUCT_SDICT_H
#define GSD_STRUCT_SDICT_H

#include <stdint.h>
#include <stdlib.h>

#include "gsd_struct_types.h"
#include "gsd_struct_dict.h"

typedef int(dict_handler)(char *key, void *val, void *args);

// Constructors
sdict *sdict_create();

sdict *sdict_clone          (sdict *d, int reference, size_t threads);
sdict *sdict_clone_immutable(sdict *d, size_t threads);

// Modifiers
result *sdict_reset(sdict *d, size_t slots, size_t max_imbalance);

// Combining
result sdict_merge(sdict *orig, sdict *dest, dict_merge_op op, int reference, size_t threads);

// Cleanup
result sdict_rebalance(sdict *d, size_t threads);
result sdict_free(sdict *d);

// Meta/settings/etc
void     *sdict_meta     (sdict *d);
size_t    sdict_slots    (sdict *d);
size_t    sdict_imbalance(sdict *d);
refdelta *sdict_refdelta (sdict *d);

// Utility
int sdict_iterate(sdict *d, sdict_handler *h, void *args);

// Operations

/* get for a given key */
result sdict_get    (sdict *d, char *key); // Get the value for a given key
result sdict_get_ref(sdict *d, char *key); // Get the ref   for a given key

/* insert for a given key, fails if it exists already */
result sdict_insert    (sdict *d, char *key, void      *val); // Insert a new key+value Fail if the key exists
result sdict_insert_ref(sdict *d, char *key, reference *ref); // Insert a new key+ref   Fail if the key exists

/* set for a given key, it may or may not exist already */
result sdict_set          (sdict *d, char *key, void      *val); // Set the value for the given key
result sdict_set_ref      (sdict *d, char *key, reference *ref); // Set the ref   for the given key
result sdict_fetch_set    (sdict *d, char *key, void      *val); // Set the value and return the previous one
result sdict_fetch_set_ref(sdict *d, char *key, reference *ref); // Set the ref   and return the previous one

/* update for a given key, fails if it does not exist */
result sdict_update          (sdict *d, char *key, void      *val); // Update the value for the given key
result sdict_update_ref      (sdict *d, char *key, reference *ref); // Update the ref   for the given key
result sdict_fetch_update    (sdict *d, char *key, void      *val); // Set the value and return the previous one
result sdict_fetch_update_ref(sdict *d, char *key, reference *val); // Set the ref   and return the previous one
result sdict_cmp_update      (sdict *d, char *key, void      *old_val, void      *new_val); // Atomic swap
result sdict_cmp_update_ref  (sdict *d, char *key, reference *old_ref, reference *new_ref); // Atomic swap

/* clear the value for a given key (the reference remains in the sdict) */
result sdict_clear      (sdict *d, char *key); // Clear the value for the ref at the given key
result sdict_fetch_clear(sdict *d, char *key); // Clear the value at the given key, and return it.
result sdict_cmp_clear  (sdict *d, char *key, void *old_val); // Atomic

/* remove the ref for a given key */
result sdict_deref      (sdict *d, char *key); // Remove the ref for a given key
result sdict_fetch_deref(sdict *d, char *key); // Remove the ref at the given key, and return it.
result sdict_cmp_deref  (sdict *d, char *key, reference *old_ref); // Atomic

#endif
