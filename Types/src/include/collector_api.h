#ifndef GSD_GC_COLLECTOR_API_H
#define GSD_GC_COLLECTOR_API_H

#include <stdint.h>
#include "type_api.h"

typedef struct object     object;
typedef struct collector  collector;
typedef struct collection collection;

collector *collector_spawn (               );
void       collector_pause ( collector *cr );
void       collector_free  ( collector *cr );

collection *gc_get_collection ( collector *cr                 );
void        gc_ret_collection ( collector *cr, collection *cn );

uint32_t gc_add_ref( object *o );
uint32_t gc_del_ref( object *o );

// Get the type-object for the specified primitive type
object *gc_get_type ( primitive p );

#endif
