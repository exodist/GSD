#include "trigger.h"
#include "assert.h"

trig_ref *trig_ref_create(trigger *trig, void *arg) {
    assert(trig);
    trig_ref *tr = malloc(sizeof(trig_ref));
    if (!tr) return NULL;
    tr->trig     = trig;
    tr->arg      = arg;
    tr->refcount = 1;

    return tr;
}

size_t trig_ref_delta(trig_ref *tr, int delta) {
    size_t count = __atomic_add_fetch(&(tr->refcount), delta, __ATOMIC_ACQ_REL);
    return count;
}

void trig_ref_free(trig_ref *tr, refdelta *rd) {
    if(tr->arg && rd) rd(tr->arg, -1);
    free(tr);
}
