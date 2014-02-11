#include "../include/gsd_struct_types.h"
#include "../include/gsd_struct_ref.h"
#include <assert.h>

void result_discard(result r) {
    switch(r.item_type) {
        case RESULT_ITEM_USER:
            if(r.item.user.val && r.item.user.rd)
                r.item.user.rd(r.item.user.val, -1);
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

void *result_get_val(result r) {
    assert(r.item_type == RESULT_ITEM_USER);
    return r.item.user.val;
}

refdelta *result_get_dlt(result r) {
    assert(r.item_type == RESULT_ITEM_USER);
    return r.item.user.rd;
}

ref *result_get_ref(result r) {
    assert(r.item_type == RESULT_ITEM_REF);
    return r.item.ref;
}

int64_t result_get_num(result r) {
    assert(r.item_type == RESULT_ITEM_NUM);
    return r.item.num;
}

