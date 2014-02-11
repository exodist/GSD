#ifndef REFERENCE_REFERENCE_H
#define REFERENCE_REFERENCE_H

#include "../include/gsd_struct_ref.h"
#include "../common/sref.h"

struct ref {
    sref     *sr;
    refdelta *rd;
};

ref *ref_from_sref(sref *sr, refdelta *rd);

#endif
