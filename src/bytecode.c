#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bytecode.h"

presub *bc_to_presub( bytecode *bc ) {
    if (!bc_push_op( bc, OP_END ))
        return NULL;

    // Shrink to just what we need.
    if ( bc->entry_idx != bc->entry_size ) {
        void *new = realloc( bc->entry, bc->entry_idx * sizeof(subop) );
        if ( new ) bc->entry = new;
    }
    if ( bc->data_idx != bc->data_size ) {
        void *new = realloc( bc->data, bc->data_idx * sizeof(object *) );
        if ( new ) bc->data = new;
    }

    presub *p = malloc( sizeof( presub ));
    if ( !p ) return NULL;
    memset( p, 0, sizeof( presub ));

    p->entry = bc->entry;
    p->data  = bc->data;
    free( bc );
    return p;
}


bytecode *new_bytecode() {
    bytecode *bc = malloc( sizeof( bytecode ));
    if ( !bc ) return bc;
    memset( bc, 0, sizeof( bc ));
    return bc;
}

int bc_push_op( bytecode *bc, uint8_t op ) {
    assert( bc );

    if ( !bc->entry_size || bc->entry_idx == bc->entry_size) {
        size_t ns = (bc->entry_size + 100);
        void *new = realloc( bc->entry, sizeof( subop ) * ns );
        if ( !new ) return 0;
        bc->entry = new;
        bc->entry_size = ns;
    }

    bc->entry[bc->entry_idx++] = op;
    return 1;
}

int bc_push_arg( bytecode *bc, unsigned int l, int source ) {
    union {
        oparg arg;
        uint8_t i;
    } a;
    a.arg.lookup = l;
    a.arg.source = source;
    return bc_push_op( bc, a.i );
}

int bc_push_data( bytecode *bc, object *o ) {
    assert( bc );

    if ( !bc->data_size || bc->data_idx == bc->data_size) {
        size_t ns = (bc->data_size + 100);
        void *new = realloc( bc->data, sizeof( object * ) * ns );
        if ( !new ) return 0;
        bc->data = new;
        bc->data_size = ns;
    }

    bc->data[bc->data_idx++] = o;
    return 1;
}
