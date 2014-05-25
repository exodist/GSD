#ifndef COMMON_SREF_H
#define COMMON_SREF_H

#include "../include/gsd_struct_types.h"
#include "../common/trigger.h"

// ******* WARNING *******
// With exception of sref_free and sref_dispose, NONE of these functions will
// modify the refcount of the xtrn pointer. If you use these, and are using
// refcount, you MUST adjust the refcount of the xtrn's according to the result
// of the actions. Items that swap xtrns will returnt he one that has been
// replaced so that you can do this.

typedef struct sref sref;

struct sref {
    size_t refcount;
    void  *xtrn;

    trig_ref *trig;
};

// Newly created sref will have a refcount of 0
sref *sref_create(void *xtrn, trig_ref *t);
size_t sref_delta(sref *sr, int delta);

void sref_free   (sref *sr, refdelta *rd);
void sref_dispose(void *sr, void *delta);

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
