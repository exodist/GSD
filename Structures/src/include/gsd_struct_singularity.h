#ifndef GSD_STRUCT_SINGULARITY_H
#define GSD_STRUCT_SINGULARITY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"
#include "../../../GC/src/include/gsd_gc.h"

singularity *singularity_create(collector *c, pool *threads, pool *prms, alloc *pointers, int8_t hasherc, ...);

collector *singularity_collector(singularity *s);

pool *singularity_threads(singularity *s);
pool *singularity_prms   (singularity *s);

alloc *singularity_pointers(singularity *s);

int8_t  singularity_hasherc(singularity *s);
hasher *singularity_hasher (singularity *s, int8_t id);

// Negative value is returned on error
int8_t singularity_hasher_push(singularity *s, hasher *h);

#endif
