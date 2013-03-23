#include <stdlib.h>
#include <string.h>

#include "structure.h"
#include "types.h"
#include "memory.h"

object *alloc_object( object *th, object *type_o, void *data ) {
    thread *t = th->data;
    object *o = malloc( sizeof( object ));
    if ( !o ) return NULL;
    memset( o, 0, sizeof( object ));
    o->type = type_o;
    o->data = data;

    // If scalar calculate content hash
    if ( type_o && type_o == t->instance->scalar_t ) {
        scalar *s = o->data;
        void *ptr    = NULL;
        size_t   size = 0;
        switch( s->init_as ) {
            case SET_AS_INT:
                ptr  = &(s->integer);
                size = sizeof( int64_t );
            break;
            case SET_AS_DEC:
                ptr  = &(s->decimal);
                size = sizeof( double );
            break;
            case SET_AS_STR:
                ptr  = s->string->string;
                size = s->string->size;
            break;
        }
        o->hash = hash_bytes( ptr, size, HASH_SEED );
    }
    else {
        o->hash = hash_bytes(
            (uint8_t *)&o,
            sizeof( object ),
            HASH_SEED
        );
    }

    if ( type_o ) {
        type *tp = type_o->data;
        if ( !tp->refcounted ) {
            int success = 0;
            while ( !success ) {
                o->gc_next = t->alloc;
                success = __sync_bool_compare_and_swap( &(t->alloc), o->gc_next, o );
            }
        }
    }

    return o;
}
