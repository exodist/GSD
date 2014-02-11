#include "reference.h"
#include "../common/trigger.h"
#include "../common/sref.h"
#include <assert.h>

ref *ref_create(void *val, refdelta *d, trigger *t, void *t_arg) {
    trig_ref *tr = NULL;
    if(t) {
        tr = trig_ref_create(t, t_arg);
        if (!tr) return NULL;
    }
    else {
        assert(!t_arg);
    }
    
    sref *sr = sref_create(val, tr);
    if (!sr) {
        trig_ref_free(tr, d);
        return NULL;
    }

    return ref_from_sref(sr, d);
}

ref *ref_from_sref(sref *sr, refdelta *rd) {
    ref *r = malloc(sizeof(ref));
    if (!r) return NULL;

    assert(sref_delta(sr, 1));

    r->sr = sr;
    r->rd = rd;

    return r;
}

result ref_get(ref *r) {
    result out = sref_get(r->sr);
    if (!out.status) return out;

    if (r->rd) {
        if (out.item.user.val) r->rd(out.item.user.val, 1);
        out.item.user.rd = r->rd;
    }

    return out;
}

result ref_set(ref *r, void *val) {
    result out = sref_set(r->sr, val);
    if (!out.status) return out;

    // No need to adjust ref count, we are -1 from the set, but +1 from the
    // result
    if (r->rd) out.item.user.rd = r->rd;

    return out;
}

result ref_update(ref *r, void *val) {
    result out = sref_update(r->sr, val);
    if (!out.status) return out;

    // No need to adjust ref count, we are -1 from the set, but +1 from the
    // result
    if (r->rd) out.item.user.rd = r->rd;

    return out;
}

result ref_delete(ref *r) {
    result out = sref_delete(r->sr);
    if (!out.status) return out;

    // No need to adjust ref count, we are -1 from the set, but +1 from the
    // result
    if (r->rd) out.item.user.rd = r->rd;

    return out;
}

result ref_insert(ref *r, void *val) {
    return sref_update(r->sr, val);
}

void ref_free(ref *r) {
    sref_free(r->sr, r->rd);
    free(r);
}

