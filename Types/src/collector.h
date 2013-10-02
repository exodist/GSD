#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "structures.h"
#include "include/collector_api.h"

typedef struct collection collection;
typedef struct collector  collector;
typedef struct region     region;

extern collector COLLECTOR;

struct collector {
    dict *roots;

    collection *associated;
    collection *available;

    pthread_t *thread;

    object_class types[12];
};

struct region {
    size_t units;
    size_t count;
    size_t index;
    void  *start;

    region *next;
};

struct collection {
    region *simple;
    region *typed;
    region *types;

    object *free_simple;
    object *free_typed;
    object *free_types;

    collection *next;
};

// Get the type-object for the specified primitive type
object *gc_get_type ( primitive p );

collection *gc_get_collection ( collector *cr                 );
void        gc_ret_collection ( collector *cr, collection *cn );

#endif
