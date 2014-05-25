#ifndef GSD_STRUCT_ALLOC_H
#define GSD_STRUCT_ALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"
#include "gsd_struct_bitmap.h"

typedef struct alloc_iterator alloc_iterator;

alloc *alloc_create(size_t item_size, size_t chunk_count);

result alloc_store(alloc *a, object *o);

result alloc_delta(alloc *a, uint32_t id, int32_t delta);

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);

#endif
