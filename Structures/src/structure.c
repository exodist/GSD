#include "include/gsd_dict.h"

#include "structure.h"
#include "balance.h"

int iterate( dict *d, dict_handler *h, void *args ) {
    epoch *e = join_epoch( d );
    set *s = d->set;
    int stop = 0;

    for ( int i = 0; i < s->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        node *n = sl->root;
        if ( n == NULL ) continue;
        stop = iterate_node( d, n, h, args );
        if ( stop ) break;
    }

    leave_epoch( d, e );
    return stop;
}

int iterate_node( dict *d, node *n, dict_handler *h, void *args ) {
    int stop = 0;

    if ( n->left != NULL && n->left != RBLD ) {
        stop = iterate_node( d, n->left, h, args );
        if ( stop ) return stop;
    }

    usref *ur = n->usref;
    sref *sr = ur->sref;
    if ( sr != NULL && sr != RBLD ) {
        xtrn *x = sr->xtrn;
        if ( x != NULL && x != RBLD ) {
            stop = h( n->key->value, x->value, args );
            if ( stop ) return stop;
        }
    }

    if ( n->right != NULL && n->right != RBLD ) {
        stop = iterate_node( d, n->right, h, args );
        if ( stop ) return stop;
    }

    return stop;
}

