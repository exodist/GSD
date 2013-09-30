#include "dictionary.h"
#include "string.h"
#include "type.h"
#include <assert.h>

#define SEED 14695981039346656037UL
#define MUL  1099511628211

dict_methods DOBJ_METH = {
    .cmp = obj_cmp,
    .loc = obj_loc,
};

int obj_cmp( void *meta, void *key1, void *key2 ) {
    object *ko1 = key1;
    object *ko2 = key2;

    // If strings, do a string compare
    if( is_string( ko1 ) && is_string( ko2 )) {
        return string_compare( ko1, ko2 );
    }

    // If simple, do a compare of simple integer
    if( is_simple( ko1 ) && is_simple( ko2 )) {
        object_simple *s1 = key1;
        object_simple *s2 = key2;
        if ( s1->simple_data.integer > s2->simple_data.integer )
            return 1;

        if ( s1->simple_data.integer < s2->simple_data.integer )
            return -1;

        if (ko1->type > ko2->type)
            return 1;

        if (ko2->type < ko2->type)
            return -1;

        // Same value, same type
        return 0;
    }

    // Compare addresses
    if ( key1 > key2 ) return  1;
    if ( key2 > key1 ) return -1;
    return 0;
}

size_t obj_loc( size_t slot_count, void *meta, void *key ) {
    exception e;
    uint64_t hash = hash_object( key, &e );
    // todo: Uh-Oh
    assert( !e );
    return hash % slot_count; 
}

uint64_t hash_bytes( uint8_t *data, size_t length ) {
    return hash_append_bytes( data, length, SEED );
}

uint64_t hash_append_bytes( uint8_t *data, size_t length, uint64_t key ) {
    if ( length < 1 ) return key;

    for ( size_t i = 0; i < length; i++ ) {
        key ^= data[i];
        key *= MUL;
    }

    return key;
}

uint64_t hash_object ( object *o, exception *e ) {
    *e = EX_NONE;
    object_simple *os = (object_simple *)o;
    string_iterator *i;
    string_header *sh = NULL;

    switch( o->type ) {
        case GC_UNDEF: return 0;

        case GC_BOOL:
        return os->simple_data.integer ? 1 : 0;

        // Cannot use a decimal number as a hash key
        case GC_DEC:
            *e = EX_DEC_AS_HKEY;
        return 0;

        // Hash the object address
        case GC_TYPED:  
        case GC_TYPE:   
        return hash_bytes((void *)o, sizeof(object *));

        // All these use the value of the simple_data
        case GC_INT:
        case GC_POINTER:
        case GC_HANDLE: 
        case GC_DICT:   
        return hash_bytes(
            (void *)&(os->simple_data.integer),
            sizeof(int64_t)
        );

        // All strings except snip might already have a
        // hash. But all get the hash the same way when
        // there is not already a cached one.
        case GC_STRING:
        case GC_STRINGC:
        case GC_ROPE:
            sh = os->simple_data.ptr;
            if (sh->hash_set)
                return sh->hash;
            // Else build it
        case GC_SNIP:
            i = iterate_string( o );
            if (!i) {
                *e = EX_OUT_OF_MEMORY;
                return 0;
            }

            uint64_t out = SEED;
            while(!iterator_complete(i)) {
                uint8_t b = iterator_next_byte(i);
                out = hash_append_bytes( &b, 1, out );
            }

            // XXX: Does this need to be atomic?
            if(sh) {
                sh->hash     = out;
                sh->hash_set = 1;
            }
        return out;

        default:
            *e = EX_INVALID_HKEY;
        return 0;
    }
}
