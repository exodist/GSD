#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_dot.h"
#include <stdio.h>
#include <string.h>

int dict_dump_dot_start( dot *dt ) {
    return dict_dump_dot_write( dt,
        "digraph dict {\n    ordering=out\n    bgcolor=black\n    node [color=yellow,fontcolor=white,shape=egg]\n    edge [color=cyan]"
    );
}

int dict_dump_dot_slink( dot *dt, int s1, int s2 ) {
    char buffer[DOT_BUFFER_SIZE];

    // Add the slot node
    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "    s%d [color=green,fontcolor=cyan,shape=box]", s2 );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    // Link it to the last one
    if ( s1 >= 0 ) {
        int ret = snprintf(
            buffer, DOT_BUFFER_SIZE,
            "    s%d->s%d [arrowhead=none,color=yellow]\n    {rank=same; s%d s%d}",
            s1, s2, s1, s2
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        return dict_dump_dot_write( dt, buffer );
    }

    return DICT_NO_ERROR;
}

int dict_dump_dot_subgraph( dot *dt, int s, node *n ) {
    char buffer[DOT_BUFFER_SIZE];
    char *label = dt->show( n->key, n->value->value ? n->value->value->value : NULL );
    int ret = 0;

    // Link node
    //s1->10 [arrowhead="none",color=blue]
    ret = snprintf( buffer, DOT_BUFFER_SIZE,
        "    s%d->\"%s\" [arrowhead=none,color=blue]",
        s, label
    );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    // subgraph
//    ret = snprintf( buffer, DOT_BUFFER_SIZE,
//        "    subgraph cluster_s%d {\n        graph [style=dotted,color=grey]",
//        s
//    );
//    if ( ret < 0 ) return DICT_INT_ERROR;
//    ret = dict_dump_dot_write( dt, buffer );
//    if ( ret ) return ret;

    ret = dict_dump_dot_node( dt, buffer, n, label );
    if ( ret ) return ret;

//    return dict_dump_dot_write( dt, "    }" );
    return ret;
}

int dict_dump_dot_node( dot *dt, char *buffer, node *n, char *label ) {
    // This node
    char *style = n->value->value ? n->value->value->value ? ""
                                                           : "[color=pink,fontcolor=pink,style=dashed]"
                                  : "[color=red,fontcolor=red,style=dashed]";

    int ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\" %s", label, style );
    if ( ret < 0 ) return DICT_INT_ERROR;
    ret = dict_dump_dot_write( dt, buffer );
    if ( ret ) return ret;

    if ( n->value->value != NULL && n->value->value->refcount > 1 ) {
        int ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        \"%s\"->\"%p\" [color=green,style=dashed]",
            label, n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        \"%p\" [color=white,fontcolor=yellow,shape=hexagon]",
            n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;

        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "        {rank=sink; \"%p\"}",
            n->value->value
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    char *left_name  = NULL;
    char *right_name = NULL;
    node *left  = n->left;
    node *right = n->right;
    if ( right != NULL ) {
        right_name = dt->show( right->key, right->value->value ? right->value->value->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }
    else if ( left != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->right), (void *)&(n->right)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    if ( left != NULL ) {
        left_name = dt->show( left->key, left->value->value ? left->value->value->value : NULL );

        // link
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        \"%s\"->\"%s\"", label, left_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }
    else if ( right != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE,
            "\"%s\"->\"%p\"[style=dotted]\n\"%p\" [label=NULL,color=grey,fontcolor=grey]",
            label, (void *)&(n->left), (void *)&(n->left)
        );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
        if ( ret ) return ret;
    }

    free(label);

    // level
    if ( left_name != NULL && right_name != NULL ) {
        ret = snprintf( buffer, DOT_BUFFER_SIZE, "        {rank=same; \"%s\" \"%s\"}", left_name, right_name );
        if ( ret < 0 ) return DICT_INT_ERROR;
        ret = dict_dump_dot_write( dt, buffer );
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

int dict_dump_dot_write( dot *dt, char *add ) {
    size_t add_length = strlen( add );

    size_t new = dt->size + add_length + 2;
    char *nb = realloc( dt->buffer, new );
    if ( nb == NULL ) return DICT_MEM_ERROR;
    dt->buffer = nb;

    // First pass
    if ( dt->size == 0 ) dt->buffer[0] = '\0';

    // copy data into buffer
    strncat( dt->buffer, add, new );
    strncat( dt->buffer, "\n", new );
    dt->size = new;

    return DICT_NO_ERROR;
}

