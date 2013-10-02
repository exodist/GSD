#include "include/collector_api.h"
#include "collector.h"

#include "structures.h"

collector *collector_spawn (               );
void       collector_pause ( collector *cr );
void       collector_free  ( collector *cr );

collection *gc_get_collection ( collector *cr                 );
void        gc_ret_collection ( collector *cr, collection *cn );

// TODO: Make atomic
uint32_t gc_add_ref( object *o ) {
    o->ref_count++;
    return o->ref_count;
}

uint32_t gc_del_ref( object *o ) {
    o->ref_count--;
    return o->ref_count;
}

// Get the type-object for the specified primitive type
object *gc_get_type ( primitive p );

void gc_dispose( void *ptr ) {
    fprintf( stderr, "dispose() unimplemented\n" );
}
