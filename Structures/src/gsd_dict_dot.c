#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_dot.h"
#include <stdio.h>
#include <string.h>

int dict_dump_dot_start( dot *dt ) {
    return dict_dump_dot_write( dt,
        "digraph dict {\n    ordering=out\n    bgcolor=black\n    node [color=yellow,fontcolor=white,shape=egg]\n    edge [color=cyan]",
        0, 0
    );
}

int dict_dump_dot_slink( dot *dt, int s1, int s2 ) {
    char buffer[DOT_BUFFER_SIZE];

    // Add the slot node
    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "    s%d [color=green,fontcolor=cyan,shape=box]", s2 );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer, 0, 0 );
    if ( ret ) return ret;

    // Link it to the last one
    if ( s1 >= 0 ) {
        int ret = snprintf(
            buffer, DOT_BUFFER_SIZE,
            "    s%d->s%d [arrowhead=none,color=yellow]\n    {rank=same; s%d s%d}",
            s1, s2, s1, s2
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        return dict_dump_dot_write( dt, buffer, 0, 0 );
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_slotn( dot *dt, int s, node *n ) {
    char buffer[DOT_BUFFER_SIZE];
    char *label = dt->show( n->key, n->usref->sref ? n->usref->sref->value : NULL );
    int ret = 0;

    // Link node
    //s1->10 [arrowhead="none",color=blue]
    ret = snprintf( buffer, DOT_BUFFER_SIZE,
        "    s%d->\"%s\" [arrowhead=none,color=blue]",
        s, label
    );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer, 0, 0 );
    if ( ret ) return ret;

    //ret = dict_dump_dot_subgraph_start( dt );
    //if ( ret ) return ret;

    ret = dict_dump_dot_node( dt, buffer, n, label );
    if ( ret ) return ret;

    //ret = dict_dump_dot_subgraph_end( dt );

    return ret;
}

int dict_dump_dot_subgraph_start( dot *dt ) {
    char buffer[DOT_BUFFER_SIZE];
    int ret = snprintf( buffer, DOT_BUFFER_SIZE,
        "    subgraph cluster_%d {\n        graph [style=dotted,color=grey]",
        dt->cluster++
    );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer, 0, 0 );
    if ( ret ) return ret;

    return ret;
}

int dict_dump_dot_subgraph_end( dot *dt ) {
    return dict_dump_dot_write( dt, "    }", 0, 0 );
}

int dict_dump_dot_node( dot *dt, char *buffer, node *n, char *label ) {
    // This node
    char *style = n->usref->sref ? n->usref->sref->value ? ""
                                                           : "[color=pink,fontcolor=pink,style=dashed]"
                                  : "[color=red,fontcolor=red,style=dashed]";

    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\" %s", label, style );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer, 0, 0 );
    if ( ret ) return ret;

    if ( n->usref->sref != NULL && n->usref->sref->refcount > 1 ) {
        int ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "    \"%s\"->\"%p\" [color=green,style=dashed]",
            label, n->usref->sref
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 1 );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "    \"%p\" [color=white,fontcolor=yellow,shape=hexagon]",
            n->usref->sref
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 1 );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "    {rank=sink; \"%p\"}",
            n->usref->sref
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 1 );
        if ( ret ) return ret;
    }

    char *left_name  = NULL;
    char *right_name = NULL;
    node *left  = n->left;
    node *right = n->right;
    if ( right != NULL ) {
        right_name = dt->show( right->key, right->usref->sref ? right->usref->sref->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
        if ( ret ) return ret;
    }
    else if ( left != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->right), (void *)&(n->right)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
        if ( ret ) return ret;
    }

    if ( left != NULL ) {
        left_name = dt->show( left->key, left->usref->sref ? left->usref->sref->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, left_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
        if ( ret ) return ret;
    }
    else if ( right != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->left), (void *)&(n->left)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
        if ( ret ) return ret;
    }

    free(label);

    // level
    if ( left_name != NULL && right_name != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        {rank=same; \"%s\" \"%s\"}", left_name, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
        if ( ret ) return ret;
    }

    // Recurse
    if ( left != NULL ) {
        ret = dict_dump_dot_node( dt, buffer, left, left_name );
        if ( ret ) return ret;
    }
    if ( right != NULL ) {
        ret = dict_dump_dot_node( dt, buffer, right, right_name );
        if ( ret ) return ret;
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_write( dot *dt, char *add, size_t asize, int bfn ) {
    size_t add_length = asize ? asize : strlen( add );

    char  **buffer = bfn ? &(dt->buffer2) : &(dt->buffer1);
    size_t *size   = bfn ? &(dt->size2)   : &(dt->size1);

    size_t new = *(size) + add_length + 2;
    char *nb = realloc( *buffer, new );
    if ( nb == NULL ) return DICT_MEM_ERROR;
    *buffer = nb;

    // First pass
    if ( *size == 0 ) *(buffer)[0] = '\0';

    // copy data into buffer
    strncat( *buffer, add, new );
    strncat( *buffer, "\n", new );

    *size = new;

    return DICT_NO_ERROR;
}

//int dict_dump_dot_epochs( dot *dt, dict *d, set *s ) {
//    char buffer[DOT_BUFFER_SIZE];
//
//    int ret = dict_dump_dot_write( dt,
//        "    Dictionary [color=grey,fontcolor=white,shape=box]\nDictionary->s0",
//        0, 0
//    );
//    if ( ret ) return ret;
//    
//    //for ( int i = 0; i < s->settings->slot_count; i++ ) {
//    //    ret = snprintf( buffer, DOT_BUFFER_SIZE, "    Dictionary->s%i", i );
//    //    if ( ret < 0 ) return DICT_INT_ERROR;
//    //    ret = dict_dump_dot_write( dt, buffer, 0, 0 );
//    //    if ( ret ) return ret;
//    //}
//
//    for ( int i = 0; i < d->epoch_count; i++ ) {
//
//        char *color = d->epochs[i]->active ? "green" : "grey";
//        char *shape = d->epochs[i]->garbage ? "doubleoctagon" : "octagon";
//
//        ret = snprintf( buffer, DOT_BUFFER_SIZE,
//            "    epoch%i [color=%s,shape=%s]",
//            i, color, shape
//        );
//        if ( ret < 0 ) return DICT_INT_ERROR;
//        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
//        if ( ret ) return ret;
//
//        ret = snprintf( buffer, DOT_BUFFER_SIZE, "    Dictionary->epoch%i", i );
//        if ( ret < 0 ) return DICT_INT_ERROR;
//        ret = dict_dump_dot_write( dt, buffer, 0, 0 );
//        if ( ret ) return ret;
//
//        if ( i != 0 ) {
//            ret = snprintf( buffer, DOT_BUFFER_SIZE, "    {rank=min; epoch0 epoch%i}", i );
//            if ( ret < 0 ) return DICT_INT_ERROR;
//            ret = dict_dump_dot_write( dt, buffer, 0, 0 );
//            if ( ret ) return ret;
//        }
//
//        // For dependancies
//        //epoch0->epoch2 [color=blue,style=dotted,constraint=none]
//    }
//
//    return DICT_NO_ERROR;
//}

