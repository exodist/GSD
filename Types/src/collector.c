#include "include/collector_api.h"
#include "collector.h"

#include "structures.h"

void collector_start();
void collector_pause();
void collector_stop();

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
