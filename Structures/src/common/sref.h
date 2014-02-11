#ifndef COMMON_SREF_H
#define COMMON_SREF_H

#include "../include/gsd_struct_types.h"
#include "../common/magic_pointers.h"
#include "../common/trigger.h"

typedef struct sref sref;

struct sref {
    size_t refcount;
    void  *xtrn;

    trig_ref *trig;
};


sref *sref_create(void *xtrn, trig_ref *t);
size_t sref_delta(sref *sr, int delta);
void  sref_free(sref *sr, refdelta rd);

result sref_trigger_check(sref *sr, void *val);

// These return the old value in result
result sref_set(sref *sr, void *val);
result sref_update(sref *sr, void *val);
result sref_delete(sref *sr);

result sref_get(sref *sr);

// result.item is always NULL
result sref_insert(sref *sr, void *val);
result sref_cmp_swap(sref *sr, void *old, void *new);

#endif
