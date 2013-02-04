#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_dot.h"
#include <stdio.h>
#include <string.h>

const node divider;
const node *DIV = &divider;

char *dict_dump_node_label( void *meta, void *key, void *value ) {
    char *out = malloc( 60 );
    if ( out == NULL ) return NULL;

    snprintf( out, 60, "%p : %p", key, value );
    return out;
}

char *dict_do_dump_dot( dict *d, set *s, dict_dot decode ) {
    char *out = "Error Building DOT\n";

    dot dd;
    memset( &dd, 0, sizeof( dot ));

    if( dict_dump_dot_epochs( d, &dd )) goto CLEANUP;
    if( dict_dump_dot_slots( d, &dd ) ) goto CLEANUP;
    if( dict_dump_dot_end( d, &dd )   ) goto CLEANUP;

    CLEANUP:
    if( dd.nodes  ) free( dd.nodes  );
    if( dd.epochs ) free( dd.epochs );
    if( dd.slots  ) free( dd.slots  );
    if( dd.refs   ) free( dd.refs   );
    if( dd.end    ) free( dd.end    );

    return out;
}

int dict_dot_print_epochs( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    int out = dict_dot_print( &(d->epochs), &(d->epochs_size), &(d->epochs_length), format, args );
    va_end( args );
    return out;
}

int dict_dot_print_slots( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    int out = dict_dot_print( &(d->slots), &(d->slots_size), &(d->slots_length), format, args );
    va_end( args );
    return out;
}

int dict_dot_print_refs( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    int out = dict_dot_print( &(d->refs), &(d->refs_size), &(d->refs_length), format, args );
    va_end( args );
    return out;
}

int dict_dot_print_end( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    int out = dict_dot_print( &(d->end), &(d->end_size), &(d->end_length), format, args );
    va_end( args );
    return out;
}

int dict_dot_print( char **buffer, size_t *size, size_t *length, char *format, va_list args ) {
    // Get the new content
    char tmp[DOT_BUFFER_INC];
    int add_length = vsnprintf( tmp, DOT_BUFFER_INC, format, args );

    size_t req = *length + add_length + 1;
    if ( *size < req ) {
        size_t new = *size;
        while ( new < req ) new += DOT_BUFFER_INC;
        char *nb = realloc( *buffer, new );
        if ( nb == NULL ) return DICT_MEM_ERROR;
        *buffer = nb;
        if ( *length == 0 ) nb[0] = '\0';
    }

    strncpy( *buffer + *length, tmp, add_length );

    return DICT_NO_ERROR;
}

int dict_dump_dot_epochs( dict *d, dot *dd ) {

}

int dict_dump_dot_slots( dict *d, dot *dd ) {

}

int dict_dump_dot_end( dict *d, dot *dd ) {

}

