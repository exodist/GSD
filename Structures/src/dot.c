#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "structure.h"
#include "node_list.h"
#include "dot.h"
#include "alloc.h"
#include "balance.h"

node NODE_SEP = { { 0 }, 0 };
// Used for 'refs' mapping
dict_settings dset = { 11, 3, NULL };
dict_methods dmet = {
    dump_dot_ref_cmp,
    dump_dot_ref_loc,
    NULL,
    NULL,
};

char *dump_dot( dict *d, dict_dot *decode ) {
    epoch *e = join_epoch( d );
    set *s = d->set;

    if( !decode ) decode = dump_node_label;

    char *out = do_dump_dot( d, s, decode );

    leave_epoch( d, e );
    return out;
}

char *dump_node_label( void *key, void *value ) {
    char *out = malloc( 60 );
    if ( out == NULL ) return NULL;

    snprintf( out, 60, "%p : %p", key, value );
    return out;
}

char *do_dump_dot( dict *d, set *s, dict_dot decode ) {
    char *out = NULL;

    dot dd;
    memset( &dd, 0, sizeof( dot ));
    dd.decode = decode ? decode : dump_node_label;
    dd.nl = nlist_create();
    if ( dd.nl == NULL ) goto DO_DUMP_DOT_CLEANUP;

    //if( dump_dot_slots( s, &dd ).num)  goto DO_DUMP_DOT_CLEANUP;
    if( dump_dot_epochs( d, &dd ).num) goto DO_DUMP_DOT_CLEANUP;

    out = dump_dot_merge( &dd );

    DO_DUMP_DOT_CLEANUP:
    if( dd.refs ) free( dd.refs );
    if( dd.slots ) free( dd.slots );
    if( dd.nodes ) free( dd.nodes );
    if( dd.epochs ) free( dd.epochs );
    if( dd.slot_level ) free( dd.slot_level );
    if( dd.node_level ) free( dd.node_level );

    return out;
}

rstat dot_print_epochs( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->epochs), &(d->epochs_size), &(d->epochs_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print_slots( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->slots), &(d->slots_size), &(d->slots_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print_nodes( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->nodes), &(d->nodes_size), &(d->nodes_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print_node_level( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->node_level), &(d->node_level_size), &(d->node_level_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print_slot_level( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->slot_level), &(d->slot_level_size), &(d->slot_level_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print_refs( dot *d, char *format, ... ) {
    va_list args;
    va_start( args, format );
    rstat out = dot_print( &(d->refs), &(d->refs_size), &(d->refs_length), format, args );
    va_end( args );
    return out;
}

rstat dot_print( char **buffer, size_t *size, size_t *length, char *format, va_list args ) {
    // Get the new content
    //char *tmp = NULL;
    //size_t add_length = 0;
    //size_t bsize = 0;
    //while ( add_length == 0 || add_length >= bsize - 1 ) {
    //    bsize += DOT_BUFFER_INC;
    //    if ( tmp != NULL ) free( tmp );
    //    tmp = malloc( bsize );
    //    memset( tmp, 0, bsize );
    //    if ( tmp == NULL ) return rstat_mem;
    //    add_length = vsnprintf( tmp, bsize, format, args );
    //}
    char tmp[DOT_BUFFER_INC];
    int add_length = vsnprintf( tmp, DOT_BUFFER_INC, format, args );
    if ( add_length >= DOT_BUFFER_INC - 1 )
        return rstat_mem;

    size_t req = *length + add_length + 1;
    if ( *size < req ) {
        size_t new = *size;
        while ( new < req ) new += DOT_BUFFER_INC;
        char *nb = realloc( *buffer, new );
        if ( nb == NULL ) return rstat_mem;
        *buffer = nb;
        memset( *buffer + *length, 0, new - *size );
        *size = new;
    }

    char *start = *buffer + *length;
    strncpy( start, tmp, add_length );
    *length += add_length;

    return rstat_ok;
}

rstat dump_dot_epochs( dict *d, dot *dd ) {
    rstat ret = dot_print_epochs( dd,
        "node [color=pink,fontcolor=grey,style=dashed,shape=octagon]\n"
    );
    if ( ret.num ) return ret;

    ret = dot_print_epochs( dd, "edge [color=cyan]\n" );
    if ( ret.num ) return ret;

    epoch *e = d->epochs;
    epoch *a = d->epoch;
    size_t en = 0;
    while ( e != NULL ) {
        epoch *dep = e->dep;

        if ( dep || e == a ) {
            char *style = "style=solid,fontcolor=white";
            char *color = ( e == a ) ? "yellow" : "green";
            char *shape = dep ? ",shape=doubleoctagon" : "";

            ret = dot_print_epochs( dd,
                "\"%p\" [label=\"Epoch%i\",%s,color=%s%s]\n",
                e, en, style, color, shape
            );
            if ( ret.num ) return ret;

            if ( dep ) {
                ret = dot_print_epochs( dd, "\"%p\"->\"%p\" [color=yellow]\n", e, dep );
                if ( ret.num ) return ret;
            }
        }
        else {
            ret = dot_print_epochs( dd, "\"%p\" [label=\"Epoch%i\"]\n", e, en );
            if ( ret.num ) return ret;
        }

        if ( e->next ) {
            ret = dot_print_epochs( dd, "\"%p\"->\"%p\"\n", e, e->next );
            if ( ret.num ) return ret;
        }

        e = e->next;
        en++;
    }

    return rstat_ok;
}

dict *dump_dot_create_refs() {
    dict *refs;
    if( do_create( &refs, 4, &dset, &dmet ).num ) {
        if ( refs != NULL ) do_free( &refs );
        return NULL;
    }

    return refs;
}

rstat dump_dot_slots_slot( dot *dd, slot *sl, size_t id, size_t previous ) {
    rstat ret;
    if ( dd->first_slot_set ) {
        ret = dot_print_slots( dd, "S%zi->S%zi\n", previous, id );
        if ( ret.num ) return ret;
    }
    else {
        dd->first_slot = id;
        dd->first_slot_set = 1;
    }

    ret = dot_print_slots( dd, "S%zi->\"%p\" [color=blue]\n", id, sl->root );
    if ( ret.num ) return ret;

    return dot_print_slot_level( dd, "S%zi ", id );
}

rstat dump_dot_slots_node( dot *dd, node *n ) {
    assert( n != RBLD );
    rstat ret;

    usref *ur = n->usref;
    sref *sr = ur->sref;
    size_t ref_count = sr ? sr->refcount : 0;

    //if ( ref_count > 1 ) {
    //    // Find the list of nodes for this ref;
    //    nlist *nl = NULL;
    //    ret = get( dd->ref_tracker, sr, (void **)&nl );
    //    if ( ret.num ) return ret;

    //    if ( nl == NULL ) {
    //        nl = nlist_create();
    //        if ( nl == NULL ) return rstat_mem;
    //        ret = set( dd->ref_tracker, sr, nl );
    //    }

    //    ret = nlist_push( nl, n );
    //    if ( ret.num ) return ret;
    //}

    ret = dot_print_node_level( dd, "\"%p\" ", n );
    if ( ret.num ) return ret;

    // print node with name
    char *name = dd->decode(
        n->key,
        n->usref->sref && n->usref->sref->xtrn ? n->usref->sref->xtrn->value : NULL
    );

    char *style = ref_count > 0
        ? ref_count > 1 ? ",peripheries=2"
                        : sr->xtrn ? ""
                                   : ",color=red,style=dashed"
        : ",color=pink,style=dashed";

    ret = dot_print_nodes( dd, "\"%p\" [label=\"%s\"%s]\n", n, name, style );
    free( name );
    if ( ret.num ) return ret;

    // print node to right
    if ( n->right && n->right != RBLD ) {
        ret = dot_print_nodes( dd, "\"%p\"->\"%p\"\n", n, n->right );
        if ( ret.num ) return ret;

        ret = nlist_push( dd->nl, n->right );
        if ( ret.num ) return ret;
    }

    // print node to left
    if ( n->left && n->left != RBLD ) {
        ret = dot_print_nodes( dd, "\"%p\"->\"%p\"\n", n, n->left );
        if ( ret.num ) return ret;

        ret = nlist_push( dd->nl, n->left );
        if ( ret.num ) return ret;
    }

    return rstat_ok;
}

rstat dump_dot_slots_nodes( dot *dd, slot *sl, size_t id ) {
    rstat ret = nlist_push( dd->nl, sl->root );
    if ( ret.num ) return ret;

    ret = nlist_push( dd->nl, &NODE_SEP );
    if ( ret.num ) return ret;

    ret = dot_print_node_level( dd, "{rank=same; " );
    if ( ret.num ) return ret;

    char *ghead = "subgraph cluster_s%zi {\n"
                  "graph [style=invisible]\n"
                  "node [color=yellow,fontcolor=white,shape=egg]\n"
                  "edge [color=cyan,arrowhead=vee]\n";

    ret = dot_print_nodes( dd, ghead, id );
    if ( ret.num ) return ret;

    node *n = nlist_shift( dd->nl );
    while ( n != NULL ) {
        // We need to change levels, end the rank set
        if ( n == &NODE_SEP ) {
            ret = dot_print_node_level( dd, "}\n" );
            if ( ret.num ) return ret;

            ret = dot_print_nodes( dd, "%s\n", dd->node_level );
            if ( ret.num ) return ret;

            memset( dd->node_level, 0, dd->node_level_length );
            dd->node_level_length = 0;

            // If more nodes remain, we need to start a new rank set.
            if ( dd->nl->first ) {
                ret = dot_print_node_level( dd, "{rank=same; " );
                if ( ret.num ) return ret;
                ret = nlist_push( dd->nl, &NODE_SEP );
                if ( ret.num ) return ret;
            }
        }
        else {
            ret = dump_dot_slots_node( dd, n );
            if ( ret.num ) return ret;
        }

        n = nlist_shift( dd->nl );
    }

    return dot_print_nodes( dd, "}\n" );
}

rstat dump_dot_slots( set *s, dot *dd ) {
    rstat ret = rstat_ok;
    dd->null_counter = 1;
    dd->ref_tracker = dump_dot_create_refs();
    if ( dd->ref_tracker == NULL ) return rstat_mem;

    ret = dot_print_slot_level( dd, "{rank=same; " );
    if ( ret.num ) goto DUMP_DOT_SLOTS_CLEANUP;

    size_t previous = 0;
    for ( size_t i = 0; i < s->settings->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        if ( sl->root == RBLD ) continue;
        
        ret = dump_dot_slots_slot( dd, sl, i, previous );
        if ( ret.num ) goto DUMP_DOT_SLOTS_CLEANUP;

        ret = dump_dot_slots_nodes( dd, sl, i );
        if ( ret.num ) goto DUMP_DOT_SLOTS_CLEANUP;

        previous = i;
    }

    ret = dot_print_slot_level( dd, "}\n" );
    if ( ret.num ) goto DUMP_DOT_SLOTS_CLEANUP;

    iterate( dd->ref_tracker, dump_dot_ref_handler, dd );
    if ( dd->refs == NULL ) {
        dd->refs = malloc( 1 );
        dd->refs[0] = '\0';
    }

    DUMP_DOT_SLOTS_CLEANUP:

    if ( dd->nl != NULL ) nlist_free( &(dd->nl) );

    if ( dd->ref_tracker != NULL ) {
        iterate( dd->ref_tracker, dump_dot_ref_free_handler, NULL );
        do_free( &(dd->ref_tracker) );
    }

    return ret;
}

char *dump_dot_merge( dot *dd ) {
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
                   //"    subgraph cluster_slots {\n"
                   //"        graph [style=solid,color=grey]\n"
                   //"        node [color=green,fontcolor=cyan,shape=rectangle]\n"
                   //"        edge [color=yellow,arrowhead=none]\n"
                   //"%s\n"
                   //"        node [color=green,fontcolor=cyan,shape=rectangle]\n"
                   //"        edge [color=yellow,arrowhead=none]\n"
                   //"%s\n"
                   //"%s\n"
                   //"%s\n"
                   //"    }\n"
                   //"    edge [constraint=none,dir=none,arrowhead=none,style=dotted,color=green]\n"
                   //"%s\n"
                   "}\n";

    size_t size = strlen( format )
                + dd->epochs_length
                + dd->nodes_length
                + dd->slots_length
                + dd->node_level_length
                + dd->slot_level_length
                + dd->refs_length
                + 10; // Padding
    char *out = malloc( size );
    if ( out == NULL ) return NULL;

    snprintf( out, size, format,
        dd->epochs
        //dd->nodes,
        //dd->slots,
        //dd->node_level,
        //dd->slot_level,
        //dd->refs
    );

    return out;
}

int dump_dot_ref_cmp( dict_settings *s, void *key1, void *key2 ) {
    if ( key1 == key2 ) return 0;
    if ( key1 > key2 ) return 1;
    return -1;
}

size_t dump_dot_ref_loc( dict_settings *s, void *key ) {
    size_t num = (size_t)key;
    return num % s->slot_count;
}

int dump_dot_ref_free_handler( void *key, void *value, void *args ) {
    nlist *nl = value;
    if ( value != NULL ) nlist_free( &nl );
    return 0;
}

int dump_dot_ref_handler( void *key, void *value, void *args ) {
    nlist *nl = value;
    dot *dd = args;

    nlist_item *first = nl->first;
    while ( first != NULL ) {
        nlist_item *pair = first->next;
        while ( pair != NULL ) {
            rstat ret = dot_print_refs( dd, "\"%p\"->\"%p\"\n", first->node, pair->node );
            if ( ret.num ) return 1;
            pair = pair->next;
        }
        first = first->next;
    }

    return 0;
}

