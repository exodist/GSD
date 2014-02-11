#include "sref.h"
#include <assert.h>

sref *sref_create(void *xtrn, trig_ref *t) {
    sref *new_sref = malloc(sizeof(sref));
    if (!new_sref) return NULL;

    new_sref->refcount = 0;
    new_sref->xtrn     = xtrn;
    new_sref->trig     = t;

    return new_sref;
}

size_t sref_delta(sref *sr, int delta) {
    size_t count = __atomic_add_fetch(&(sr->refcount), delta, __ATOMIC_ACQ_REL);
    return count;
}

void sref_free(sref *sr, refdelta *rd) {
    if (rd && sr->xtrn && !blocked_null(sr->xtrn))
        rd(sr->xtrn, -1);

    if (sr->trig) {
        size_t count = trig_ref_delta(sr->trig, -1);
        if (!count) trig_ref_free(sr->trig, rd);
    }

    free(sr);
}

result sref_trigger_check(sref *sr, void *val) {
    return sr->trig->trig(sr->trig->arg, val);
}

// These return the old value in result

result sref_set(sref *sr, void *val) {
    result out = sref_trigger_check(sr, val);

    if (out.status) {
        out.item_type = RESULT_ITEM_USER;
        __atomic_exchange(&(sr->xtrn), &val, &(out.item.user.val), __ATOMIC_ACQ_REL);
    }

    return out;
}

result sref_update(sref *sr, void *val) {
    result out = sref_trigger_check(sr, val);
    if (!out.status) return out;

    out = sref_get(sr);

    while(1) {
        if (!out.item.user.val) {
            out.status = RES_FAILURE;
            return out;
        }

        int ok = __atomic_compare_exchange(
            &(sr->xtrn),
            &(out.item.user.val),
            &val,
            0,
            __ATOMIC_RELEASE,
            __ATOMIC_CONSUME
        );

        if (ok) return out;
    }
}

result sref_delete(sref *sr) {
    return sref_update(sr, NULL);
}

result sref_cmp_swap(sref *sr, void *old, void *new) {
    result out = sref_trigger_check(sr, new);
    if (!out.status) return out;

    int ok = __atomic_compare_exchange(
        &(sr->xtrn),
        &old,
        &new,
        0,
        __ATOMIC_RELEASE,
        __ATOMIC_CONSUME
    );

    if (ok) return out;

    out.status = RES_FAILURE;
    return out;
}

result sref_get(sref *sr) {
    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_USER,
        .item.user.val = NULL,
        .item.user.rd  = NULL
    };

    __atomic_load(&(sr->xtrn), &(out.item.user.val), __ATOMIC_CONSUME);
    return out;
}

result sref_insert(sref *sr, void *val) {
    result out = sref_trigger_check(sr, val);
    if (!out.status) return out;

    out = sref_get(sr);
    out.item_type = RESULT_ITEM_NONE;

    while(1) {
        if (out.item.user.val) {
            out.status = RES_FAILURE,
            out.item.user.val = NULL;
            return out;
        }

        assert(out.item.user.val == NULL);
        int ok = __atomic_compare_exchange(
            &(sr->xtrn),
            &(out.item.user.val),
            &val,
            0,
            __ATOMIC_RELEASE,
            __ATOMIC_CONSUME
        );

        if (ok) return out;
    }
}

