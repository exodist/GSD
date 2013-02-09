
//-------------------
// These functions are defined in gsd_dict.h
// They are publicly exposed functions.
// Changing how these work requires a major version bump.
//-------------------

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    epoch *e = dict_join_epoch( d );
    set *s = d->set;
    int stop = DICT_NO_ERROR;

    for ( int i = 0; i < s->settings->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        node *n = sl->root;
        if ( n == NULL ) continue;
        stop = dict_iterate_node( d, n, h, args );
        if ( stop ) break;
    }

    dict_leave_epoch( d, e );
    return stop;
}

//------------------------------------------------
// Nothing below here is publicly exposed.
//------------------------------------------------

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args ) {
    int stop = 0;

    if ( n->left != NULL && n->left != RBLD ) {
        stop = dict_iterate_node( d, n->left, h, args );
        if ( stop ) return stop;
    }

    usref *ur = n->usref;
    sref *sr = ur->sref;
    if ( sr != NULL && sr != RBLD ) {
        if ( sr->value != NULL ) {
            stop = h( n->key, sr->value, args );
            if ( stop ) return stop;
        }
    }

    if ( n->right != NULL && n->right != RBLD ) {
        stop = dict_iterate_node( d, n->right, h, args );
        if ( stop ) return stop;
    }

    return DICT_NO_ERROR;
}

