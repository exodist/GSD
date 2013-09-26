#include "dictionary.h"

#define SEED 14695981039346656037UL
#define MUL  1099511628211

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

uint64_t hash_object ( object *o ) {
    // These will get a value-specific hash:
    //    GC_POINTER
    //    GC_INT
    //    GC_CHARS
    //    GC_HANDLE
    //    GC_DICT
    //    GC_STRING
    //    GC_STRINGF
    //    GC_BOOL

    // Constant hash:
    //    GC_UNDEF

    // Object address hash:
    //    GC_TYPED
    //    GC_TYPE

    // Throws an exception
    //    GC_DEC

}
