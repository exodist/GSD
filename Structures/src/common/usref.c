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
    while(1) {
        result check = usref_check(usr);
        if (!check.status) return check;

        sref *sr = result_get_ptr(check);

        if (sr) return sref_set(sr, val);

        sref *new = sref_create(val, NULL);
        if (!new) {
            out.status = RES_FAILURE;
            out.error  = RES_OUT_OF_MEMORY;
            return out;
        }
        result out = usref_do_set_sref(usr, new, 1);
        if (out.status) return out;
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

static result usref_do_set_sref(usref *usr, sref *val, int null_switch) {
    sref *sr = NULL;
    __atomic_load(&(usr->sref), &sr, __ATOMIC_CONSUME);

    result out = {
        .status    = RES_SUCCESS,
        .error     = RES_NO_ERROR,
        .message   = NULL,
        .item_type = RESULT_ITEM_NONE,
        .item.num  = 0,
    };

    while(1) {
        if (blocked_null(sr)) {
            out.status    = RES_FAILURE;
            out.error     = RES_BLOCKED;
            out.message   = "Internal Error, this should have been caught...";
            out.item_type = RESULT_ITEM_PTR;
            out.item.ptr  = sr;
            return out;
        }

        int null_status = 0;
        switch(null_switch) {
            case -1: null_status = sr != NULL; break;
            case  1: null_status = sr == NULL; break;
            case  0: null_status = 1;          break;
            default: assert( ok );
        }

        if (!null_status) {
            out.status = RES_FAILURE;
            return out;
        }

        int ok = __atomic_compare_exchange(
            &(usr->sref),
            &sr,
            &val,
            0,
            __ATOMIC_ACQ_REL,
            __ATOMIC_CONSUME
        );

        if (!ok) continue;

        sref_delta(val, 1);
        //sref_delta(sr, -1); // Disabled because we are returning it, -1 for
        //                       removal, but +1 for return.

        out.item_type = RESULT_ITEM_PTR;
        out.item.ptr  = sr;

        return out;
    }
}

result usref_set_sref(usref *usr, sref *val) {
    assert(val);
    return usref_do_set_sref(usr, val, 0);
}

result usref_update_sref(usref *usr, sref *val) {
    assert(val);
    return usref_do_set_sref(usr, val, -1);
}

result usref_get_sref(usref *usr) {

}

result usref_unlink(usref *usr) {
    return usref_do_set_sref(usr, NULL, -1);
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


