#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_dot.h"
#include <stdio.h>
#include <string.h>

char *dict_dump_node_label( void *key, void *value ) {
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

    if( dict_dump_dot_epochs( d, &dd )) goto DO_DUMP_DOT_CLEANUP;
    if( dict_dump_dot_slots( d, &dd ))  goto DO_DUMP_DOT_CLEANUP;

    out = dict_dump_dot_merge( &dd );

    DO_DUMP_DOT_CLEANUP:
    if( dd.epochs ) free( dd.epochs );
    if( dd.slots )  free( dd.slots );
    if( dd.level )  free( dd.level );
    if( dd.refs )   free( dd.refs );

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

int dict_dot_print_level( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    int out = dict_dot_print( &(d->level), &(d->level_size), &(d->level_length), format, args );
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
        memset( *buffer + *length, 0, new - *size );
        *size = new;
    }

    char *start = *buffer + *length;
    strncpy( start, tmp, add_length );
    *length += add_length;

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

int dict_dump_dot_slots_add_node( nl **start, nl **end, void *n, int null ) {
    nl *new = malloc( sizeof( nl ));
    if ( new == NULL ) return DICT_MEM_ERROR;
    new->node = n;
    new->next = NULL;
    new->null = null;

    if ( *start == NULL ) *start = new;
    if ( *end   != NULL ) (*end)->next = new;
    *end = new;

    return DICT_NO_ERROR;
}

int IN = 0;

int dict_dump_dot_slots( dict *d, dot *dd ) {
    set *s = d->set;

    int ret = dict_dot_print_slots( dd, "node [color=green,fontcolor=cyan,shape=rectangle]\n" );
    if ( ret ) return ret;
    ret = dict_dot_print_slots( dd, "edge [color=yellow,arrowhead=none]\n" );
    if ( ret ) return ret;

    nl *nl_start = NULL;
    nl *nl_end = NULL;

    dict_settings dset = { 11, 3, NULL };
    dict_methods dmet = {
        dict_dump_dot_ref_cmp,
        dict_dump_dot_ref_loc,
        NULL,
        NULL,
    };

    dict *refs;
    if( dict_create( &refs, 4, &dset, &dmet ))
        return DICT_INT_ERROR;

    size_t nc = 1;
    for ( size_t i = 0; i < s->settings->slot_count; i++ ) {
        if ( s->slots[i] == NULL ) continue;

        if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, s->slots[i]->root, 0 ))
            goto DUMP_DOT_SLOTS_CLEANUP;

        if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, NULL, 0 ))
            goto DUMP_DOT_SLOTS_CLEANUP;

        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

        ret = dict_dot_print_level( dd, "{rank=same; " );
        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

        char *ghead = "subgraph cluster_s%zi {\n"
                      "graph [style=invisible]\n"
                      "node [color=yellow,fontcolor=white,shape=egg]\n"
                      "edge [color=cyan,arrowhead=vee]\n";

        ret = dict_dot_print_slots( dd, ghead, i );
        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

        while ( nl_start != NULL ) {
            if ( nl_start->null ) {
                ret = dict_dot_print_slots( dd, "\"null%zi\" [color=grey,style=dashed,label=NULL]\n", nl_start->null );
                if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;
            }
            else if ( nl_start->node == NULL ) {
                ret = dict_dot_print_level( dd, "}\n" );
                if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                ret = dict_dot_print_slots( dd, "%s\n", dd->level );
                if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                memset( dd->level, 0, dd->level_length );
                dd->level_length = 0;

                if ( nl_start->next ) {
                    ret = dict_dot_print_level( dd, "{rank=same; " );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;
                    if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, NULL, 0 ))
                        goto DUMP_DOT_SLOTS_CLEANUP;
                }
            }
            else { // Real node
                sref *sr = nl_start->node->usref->sref;
                size_t ref_count = sr ? sr->refcount : 0;

                if ( ref_count > 1 ) {
                    // Get existing if any
                    void *next = NULL;
                    ret = dict_get( refs, sr, &next );
                    if ( ret ) fprintf( stderr, "FAIL: %i, %i\n", __LINE__, ret );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                    nl *us = malloc( sizeof( nl ));
                    us->node = nl_start->node;
                    us->next = next;
                    us->null = 0;
                    ret = dict_set( refs, sr, us );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;
                }

                ret = dict_dot_print_level( dd, "\"%p\" ", nl_start->node );
                if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                // print node with name
                char *name = dd->decode(
                    nl_start->node->key,
                    nl_start->node->usref->sref ? nl_start->node->usref->sref->value : NULL
                );
                char *style = ref_count > 0
                    ? ref_count > 1 ? ",peripheries=2"
                                    : sr->value ? ""
                                                : ",color=red,style=dashed"
                    : ",color=pink,style=dashed";
                ret = dict_dot_print_slots( dd, "\"%p\" [label=\"%s\"%s]\n", nl_start->node, name, style );
                if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                // print node to right
                if ( nl_start->node->right ) {
                    ret = dict_dot_print_slots( dd, "\"%p\"->\"%p\"\n", nl_start->node, nl_start->node->right );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                    if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, nl_start->node->right, 0 ))
                        goto DUMP_DOT_SLOTS_CLEANUP;
                }
                else if ( nl_start->node->left ) {
                    size_t tnc = nc++;
                    ret = dict_dot_print_slots( dd, "\"%p\"->\"null%zi\"\n", nl_start->node, tnc );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                    if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, NULL, tnc ))
                        goto DUMP_DOT_SLOTS_CLEANUP;
                }

                // print node to left
                if ( nl_start->node->left ) {
                    ret = dict_dot_print_slots( dd, "\"%p\"->\"%p\"\n", nl_start->node, nl_start->node->left );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                    if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, nl_start->node->left, 0 ))
                        goto DUMP_DOT_SLOTS_CLEANUP;
                }
                else if ( nl_start->node->right ) {
                    size_t tnc = nc++;
                    ret = dict_dot_print_slots( dd, "\"%p\"->\"null%zi\"\n", nl_start->node, tnc );
                    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

                    if( dict_dump_dot_slots_add_node( &nl_start, &nl_end, NULL, tnc ))
                        goto DUMP_DOT_SLOTS_CLEANUP;
                }
            }

            nl *done = nl_start;
            nl_start = nl_start->next;
            free( done );
        }

        ret = dict_dot_print_slots( dd, "}\n" );
        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;
    }

    ret = dict_dot_print_level( dd, "{rank=same; " );
    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

    char *gend = "node [color=green,fontcolor=cyan,shape=rectangle]\n"
                 "edge [color=yellow,arrowhead=none]\n";

    ret = dict_dot_print_slots( dd, gend );
    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

    size_t previous = 0;
    for ( size_t i = 0; i < s->settings->slot_count; i++ ) {
        if ( s->slots[i] == NULL ) continue;
        if ( dd->first_slot_set ) {
            ret = dict_dot_print_slots( dd, "S%zi->S%zi\n", previous, i );
            if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;
        }
        else {
            dd->first_slot = i;
            dd->first_slot_set = 1;
        }

        ret = dict_dot_print_slots( dd, "S%zi->\"%p\" [color=blue]\n", i, s->slots[i]->root );
        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

        ret = dict_dot_print_level( dd, "S%zi ", i );
        if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

        previous = i;
    }

    ret = dict_dot_print_level( dd, "}\n" );
    if ( ret ) goto DUMP_DOT_SLOTS_CLEANUP;

    dict_iterate( refs, dict_dump_dot_ref_handler, dd );
    if ( dd->refs == NULL ) {
        dd->refs = malloc( 1 );
        dd->refs[0] = '\0';
    }

    DUMP_DOT_SLOTS_CLEANUP:

    if ( refs != NULL ) {
        dict_iterate( refs, dict_dump_dot_ref_free_handler, NULL );
        dict_free( &refs );
    }
    while ( nl_start != NULL ) {
        nl *done = nl_start;
        nl_start = nl_start->next;
        free( done );
    }

    return ret;
}

char *dict_dump_dot_merge( dot *dd ) {
    char *format = "digraph Dictionary {\n"
                   "    ordering=out\n"
                   "    bgcolor=black\n"
                   "    splines=false\n"
                   "    node [color=white,fontcolor=white,shape=egg,style=solid]\n"
                   "    edge [color=blue,arrowhead=vee]\n"
                   "    subgraph cluster_memory {\n"
                   "        graph [style=solid,color=grey]\n"
                   "%s\n"
                   "    }\n"
                   "    subgraph cluster_slots {\n"
                   "        graph [style=solid,color=grey]\n"
                   "%s\n"
                   "%s\n"
                   "    }\n"
                   "    edge [constraint=none,dir=none,arrowhead=none,style=dotted,color=green]\n"
                   "%s\n"
                   "}\n";

    size_t size = strlen( format )
                + dd->epochs_length
                + dd->slots_length
                + dd->level_length
                + dd->refs_length
                + 10; // Padding
    char *out = malloc( size );
    if ( out == NULL ) return NULL;

    snprintf( out, size, format, dd->epochs, dd->slots, dd->level, dd->refs );

    return out;
}

int dict_dump_dot_ref_cmp( dict_settings *s, void *key1, void *key2 ) {
    if ( key1 == key2 ) return 0;
    if ( key1 > key2 ) return 1;
    return -1;
}

size_t dict_dump_dot_ref_loc( dict_settings *s, void *key ) {
    size_t num = (size_t)key;
    return num % s->slot_count;
}

int dict_dump_dot_ref_free_handler( void *key, void *value, void *args ) {
    nl *mnl = value;
    while ( mnl != NULL ) {
        nl *goner = mnl;
        mnl = mnl->next;
        free( goner );
    }
    return 0;
}

int dict_dump_dot_ref_handler( void *key, void *value, void *args ) {
    nl *first = value;
    dot *dd = args;

    while ( first != NULL ) {
        nl *pair = first->next;
        while ( pair != NULL ) {
            int ret = dict_dot_print_refs( dd, "\"%p\"->\"%p\"\n", first->node, pair->node );
            if ( ret ) return ret;
            pair = pair->next;
        }
        first = first->next;
    }

    return DICT_NO_ERROR;
}

