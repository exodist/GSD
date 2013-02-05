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
    char *out = NULL;

    dot dd;
    memset( &dd, 0, sizeof( dot ));
    dd.decode = decode ? decode : dict_dump_node_label;

    if( dict_dump_dot_epochs( d, &dd )) goto CLEANUP;
    if( dict_dump_dot_slots( d, &dd )) goto CLEANUP;
    if( dict_dump_dot_end( d, &dd )) goto CLEANUP;

    out = dict_dump_dot_merge( &dd );

    CLEANUP:
    if( dd.nodes )  free( dd.nodes );
    if( dd.epochs ) free( dd.epochs );
    if( dd.slots )  free( dd.slots );
    if( dd.refs )   free( dd.refs );
    if( dd.end )    free( dd.end );

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
    *length = *length + add_length;

    return DICT_NO_ERROR;
}

int dict_dump_dot_epochs( dict *d, dot *dd ) {

    int ret = dict_dot_print_epochs( dd,
        "node [color=pink,fontcolor=grey,style=dashed,shape=octagon]\n"
    );
    if ( ret ) return ret;

    ret = dict_dot_print_epochs( dd, "edge [color=cyan]\n" );
    if ( ret ) return ret;

    epoch *e = d->epochs;
    epoch *a = d->epoch;
    size_t en = 0;
    while ( e != NULL ) {
        epoch *dep = e->dep;
        if ( dep ) {
            ret = dict_dot_print_epochs( dd,
                "\"%p\" [label=\"Epoch%i\",color=green,style=solid,shape=doubleoctagon,fontcolor=white]\n",
                e, en
            );
            if ( ret ) return ret;
            ret = dict_dot_print_epochs( dd, "\"%p\"->\"%p\" [color=yellow]\n", e, dep );
            if ( ret ) return ret;
        }
        else if ( e == a ) {
            ret = dict_dot_print_epochs( dd,
                "\"%p\" [label=\"Epoch%i\",color=yellow,style=solid,fontcolor=white]\n",
                e, en
            );
            if ( ret ) return ret;
        }
        else {
            ret = dict_dot_print_epochs( dd, "\"%p\" [label=\"Epoch%i\"]\n", e, en );
            if ( ret ) return ret;
        }

        if ( e->next ) {
            ret = dict_dot_print_epochs( dd, "\"%p\"->\"%p\"\n", e, e->next );
            if ( ret ) return ret;
        }

        e = e->next;
        en++;
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_slots( dict *d, dot *dd ) {

}

int dict_dump_dot_end( dict *d, dot *dd ) {
    int ret = dict_dot_print_end( dd,
        "Dictionary->\"%p\"\nDictionary->S0\n", d->epochs
    );
    if ( ret ) return ret;
}

char *dict_dump_dot_merge( dot *dd ) {
    char *out = malloc( dd->epochs_length + 10000 );
    if ( out == NULL ) return NULL;

    char *format = "digraph Memory {\n"
                   "    ordering=out\n"
                   "    bgcolor=black\n"
                   "    node [color=white,fontcolor=white,shape=egg,style=solid]\n"
                   "    edge [color=cyan,arrowhead=vee]\n"
                   "    subgraph cluster_memory {\n"
                   "    graph [style=solid,color=grey]\n"
                   "        %s\n"
                   "    }\n"
                   "    %s\n"
                   "}\n";

    snprintf( out, dd->epochs_length + 10000, format, dd->epochs, dd->end );

    return out;
}
