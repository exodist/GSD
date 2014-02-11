#ifndef COMMON_TRIGGER_H
#define COMMON_TRIGGER_H

#include "../include/gsd_struct_types.h"

typedef struct trig_ref trig_ref;

struct trig_ref {
    trigger *trig;
    void    *arg;
    size_t   refcount;
};

trig_ref *trig_ref_create(trigger *trig, void *arg);
size_t    trig_ref_delta(trig_ref *tr, int delta);
void      trig_ref_free(trig_ref *tr, refdelta *rd);

#endif
