#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "structures.h"
#include "include/collector_api.h"

typedef struct collection collection;
typedef struct collector  collector;

extern collector COLLECTOR;

struct collector {
};

struct collection {
};

// Get the type-object for the specified primitive type
object *gc_get_type ( primitive p );

collection *gc_get_collection ( collector *cr                 );
void        gc_ret_collection ( collector *cr, collection *cn );

#endif
