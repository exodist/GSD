#ifndef SINGULARITY_SINGULARITY_H
#define SINGULARITY_SINGULARITY_H
#include "../include/gsd_struct_singularity.h"

struct singularity {
    collector *collector;

    pool *threads;
    pool *prms;

    alloc *pointers;

    int64_t hasherc;
    hasher *hashers[100];
};


#endif
