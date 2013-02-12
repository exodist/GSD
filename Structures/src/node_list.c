#include <stdlib.h>
#include <string.h>

#include "node_list.h"

nlist *nlist_create() {
    nlist *out = malloc( sizeof( nlist ));
    if ( out == NULL ) return NULL;
    memset( out, 0, sizeof( nlist ));
    return out;
}

rstat nlist_push( nlist *nl, node *n ) {
    nlist_item *i = malloc( sizeof( nlist_item ));
    if ( i == NULL ) return rstat_mem;
    i->node = n;
    i->next = NULL;

    if ( nl->first == NULL ) nl->first = i;
    if ( nl->last != NULL )  nl->last->next = i;
    nl->last = i;

    return rstat_ok;
}

node *nlist_shift( nlist *nl ) {
    if ( nl->first == NULL ) return NULL;

    nlist_item *i = nl->first;
    nl->first = i->next;
    node *out = i->node;

    free( i );

    return out;
}

void nlist_free( nlist **nlp ) {
    nlist *nl = *nlp;
    *nlp = NULL;

    nlist_item *i = nl->first;
    while ( i != NULL ) {
        nlist_item *tmp = i;
        i = i->next;
        free( tmp );
    }

    free( nl );
}

