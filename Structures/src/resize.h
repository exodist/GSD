#ifndef RESIZE_H
#define RESIZE_H

#include <stdlib.h>
#include <stdint.h>

#include "structure.h"
#include "error.h"

rstat resize( dict *d, size_t slot_count, size_t max_threads );

void *resize_worker( void *args );

int resize_transfer_slot( set *s, size_t idx, dict *orig, dict *dest );

rstat resize_check( dict *d );

#endif
