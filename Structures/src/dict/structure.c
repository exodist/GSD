#include "include/gsd_dict.h"

#include "structure.h"
#include "balance.h"

int iterate( dict *d, dict_handler *h, void *args ) {
    uint8_t e = join_epoch( d->prm );
    set *s = d->set;
    int stop = 0;

    for ( int i = 0; i < s->settings.slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        node *n = sl->root;
        if ( n == NULL ) continue;
        stop = iterate_node( d, s, n, h, args );
        if ( stop ) break;
    }

    leave_epoch( d->prm, e );
    return stop;
}

int iterate_node( dict *d, set *s, node *n, dict_handler *h, void *args ) {
    int stop = 0;

    if ( n->left && !blocked_null( n->left )) {
        stop = iterate_node( d, s, n->left, h, args );
        if ( stop ) return stop;
    }

    void *val = NULL;
    usref *ur = n->usref;
    sref *sr = ur->sref;
    if ( sr && !blocked_null( sr )) {
        val = sr->xtrn;
    }

    if ( val && !blocked_null( val )) {
        stop = h( n->key, val, args );
        if ( stop ) return stop;
    }

    if ( n->right && !blocked_null( n->right )) {
        stop = iterate_node( d, s, n->right, h, args );
        if ( stop ) return stop;
    }

    return stop;
}

