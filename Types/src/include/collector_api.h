#ifndef GSD_GC_COLLECTOR_API_H
#define GSD_GC_COLLECTOR_API_H

#include <stdint.h>
#include "type_api.h"

typedef struct object object;

void collector_start();
void collector_pause();
void collector_stop();

uint32_t gc_add_ref( object *o );
uint32_t gc_del_ref( object *o );

void gc_dispose( void * );

#endif
