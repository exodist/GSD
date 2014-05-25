#include <assert.h>
#include "result.h"
#include "../include/gsd_struct_ref.h"

res_error_code result_error(result r) {
    return r.error;
}

res_status_code result_status(result r) {
    return r.status;
}

const char *result_message(result r) {
    return r.message;
}

void result_discard(result r) {
    switch(r.item_type) {
        case RESULT_ITEM_PTR:
            assert(0); // TODO
        return;

        case RESULT_ITEM_REF:
            ref_free(r.item.ref);
        return;

        case RESULT_ITEM_NONE:
        case RESULT_ITEM_NUM:
        default:
        return;
    }
}

ref *result_get_ref(result r) {
    assert(r.item_type == RESULT_ITEM_REF);
    return r.item.ref;
}

int64_t result_get_num(result r) {
    assert(r.item_type == RESULT_ITEM_NUM);
    return r.item.num;
}

void *result_get_ptr(result r) {
    assert(r.item_type == RESULT_ITEM_PTR);
    return r.item.ptr;
}

