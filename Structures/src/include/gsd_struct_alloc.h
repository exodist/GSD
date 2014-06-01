#ifndef GSD_STRUCT_ALLOC_H
#define GSD_STRUCT_ALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

typedef struct alloc_iterator alloc_iterator;

alloc *alloc_create(size_t item_size, size_t item_count, size_t offset, size_t ref_bytes, prm *prm);

// Returns 0 on error, 0 is never a valid index.
// Index will always be larger than the offset, this allows you to use values
// <= offset as special/magic values.
uint64_t alloc_spawn(alloc *a);               // ref count = 0
void    *alloc_fetch(alloc *a, uint64_t idx); // ref count + 1
void     alloc_deref(alloc *a, uint64_t idx); // rec count - 1

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);

#endif
