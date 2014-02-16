#ifndef COMMON_USREF_H
#define COMMON_USREF_H

#include "../common/sref.h"
#include "../include/gsd_struct_types.h"
#include "../common/magic_pointers.h"

// ******* WARNING *******
// With exception of usref_free and usref_dispose, NONE of these functions will
// modify the refcount of the xtrn pointer. If you use these, and are using
// refcount, you MUST adjust the refcount of the xtrn's according to the result
// of the actions. Items that swap xtrns will returnt he one that has been
// replaced so that you can do this.
//
// Refcounting *IS* done for the sref related functions

typedef struct usref usref;

struct usref {
    size_t refcount;
    sref *usref;
};

// Newly created usref will have a refcount of 0
usref *usref_create(sref *usr);
size_t usref_delta(usref *usr, int delta);

void usref_free   (usref *usr, refdelta *rd);
void usref_dispose(void *usr, void *delta);

result usref_check(usref *usr);

result usref_trigger_check(usref *usr, void *val);

// These return the old value in result
result usref_set(usref *usr, void *val);
result usref_update(usref *usr, void *val);
result usref_delete(usref *usr);

result usref_get(usref *usr);

result usref_set_sref(usref *usr, sref *val);
result usref_update_sref(usref *usr, sref *val);

result usref_get_sref(usref *usr);

// Result will have the sref in the ptr
result usref_unlink(usref *usr);

// Result will have the block pointer.
result usref_unblock(usref *usr);

// Result will have the sref on failure, nothing on success
result usref_block(usref *usr, void *type);

// result.item is always NULL
result usref_insert(usref *usr, void *val);
result usref_cmp_swap(usref *usr, void *old, void *new);

result usref_insert_sref(usref *usr, sref *val);
result usref_cmp_swap_sref(usref *usr, sref *old, sref *new);

static result usref_do_set_sref(usref *usr, sref *val, int null_switch) {

#endif
