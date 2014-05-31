#ifndef GSD_STRUCT_ALLOC_H
#define GSD_STRUCT_ALLOC_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "gsd_struct_types.h"

typedef struct alloc_iterator alloc_iterator;

alloc *alloc_create(size_t item_size, size_t item_count, uint8_t ref_bytes, prm *prm);

int64_t alloc_spawn(alloc *a);             // ref count =1
void   *alloc_get(alloc *a, uint32_t idx); // ref count +1
void    alloc_ret(alloc *a, uint32_t idx); // rec count -1

alloc_iterator *alloc_iterate(alloc *a);
result          alloc_iterate_next(alloc_iterator *i);
void            alloc_iterator_free(alloc_iterator *i);

#endif
