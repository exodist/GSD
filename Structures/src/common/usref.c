#include "usref.h"
#include <assert.h>

usref *usref_create(sref *sr) {
    usref *out = malloc(sizeof(usref));
    if (!out) return NULL;
    out->refcount = 0;
    out->sref = sr;
    if (sr) sref_delta(sr, 1);

    return out;
}

size_t usref_delta(usref *usr, int delta) {
    size_t count = __atomic_add_fetch(&(usr->refcount), delta, __ATOMIC_ACQ_REL);
    return count;
}

void usref_free(usref *usr, refdelta *rd) {
    sref *sr = NULL;
    __atomic_load(&(usr->sref), &sr, __ATOMIC_CONSUME);

    if (sr && !blocked_null(sr)) {
        size_t count = sref_delta(sr, -1);
        if (!count) sref_free(sr, rd);
    }

    free(usr);
}

void usref_dispose(void *usr, void *delta) {
    usref_free(usr, delta);
}

result usref_trigger_check(usref *usr, void *val) {
    sref *sr = NULL;
    __atomic_load(&(usr->sref), &sr, __ATOMIC_CONSUME);

    if (sr && !blocked_null(sr)) return sref_trigger_check(sr, val);

    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };
    return out;
}

result usref_check(usref *usr) {
     sref *sr = NULL;
    __atomic_load(&(usr->sref), &sr, __ATOMIC_CONSUME);

    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };

    if (blocked_null(sr)) {
        out.status    = RES_FAILURE;
        out.error     = RES_BLOCKED;
        out.message   = "Internal Error, this should have been caught...";
        out.item_type = RESULT_ITEM_PTR;
        out.item.ptr  = sr;
        return out;
    }

    out.item_type = RESULT_ITEM_PTR;
    out.item.ptr  = sr;
    return out;
}

// These return the old value in result
result usref_set(usref *usr, void *val) {
    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };

    result check = usref_check(usr);
    if (!check.status) return check;
    
    sref *sr = result_get_ptr(check);
    
    while (1) {
        if (sr) {
            return sref_set(sr, val);
        }
        else {
            sref *new = sref_create(val, NULL);
            if (!new) {
                out.status = RES_FAILURE;
                out.error = RES_OUT_OF_MEMORY;
                return out;
            }
            assert(sref_delta(new, 1));

            int ok =__atomic_compare_exchange(&(usr->sref), &sr, &new, 0, __ATOMIC_ACQ_REL, __ATOMIC_CONSUME);
            if (!ok) {
                assert(sref_delta(new, -1));
                sref_free(sr, NULL);
                continue;
            };
            out.item_type = RESULT_ITEM_USER; // Retuning a null val
            out.item.user.val = NULL;
            out.item.user.rd  = NULL;
            return out;
        }
    }
}

result usref_update(usref *usr, void *val) {
    result check = usref_check(usr);
    if (!check.status) return check;
    
    sref *sr = result_get_ptr(check);

    if (sr) return sref_set(sr, val);

    result out = {
        .status    = RES_FAILURE,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };

    return out;
}

result usref_delete(usref *usr) {
    result check = usref_check(usr);
    if (!check.status) return check;
    
    sref *sr = result_get_ptr(check);

    if (sr) return sref_delete(sr, val);

    result out = {
        .status    = RES_FAILURE,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };

    return out;
}

result usref_get(usref *usr) {
    result check = usref_check(usr);
    if (!check.status) return check;
    sref *sr = result_get_ptr(check);
    if (sr) return sref_get(sr);

    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_USER,
        .item.user.val = NULL,
        .item.user.rd  = NULL;
    };

    return out;
}

result usref_set_sref(usref *usr, sref *val);
result usref_update_sref(usref *usr, sref *val);
result usref_delete_sref(usref *usr);

result usref_get_sref(usref *usr);


result usref_unlink(usref *usr) {
}

result usref_unblock(usref *usr) {
}


result usref_block(usref *usr, void *type) {
}

// result.item is always NULL

result usref_insert(usref *usr, void *val) {
}

result usref_cmp_swap(usref *usr, void *old, void *new) {
}

result usref_insert_sref(usref *usr, sref *val);
result usref_cmp_swap_sref(usref *usr, sref *old, sref *new);


