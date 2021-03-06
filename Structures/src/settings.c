#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "../../PRM/src/include/gsd_prm.h"

#include "include/gsd_dict.h"

#include "devtools.h"
#include "settings.h"
#include "structure.h"
#include "node_list.h"
#include "balance.h"
#include "location.h"
#include "alloc.h"
#include "util.h"
#include "multidict.h"
#include "operations.h"
#include "magic_pointers.h"

dict_settings get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods get_methods( dict *d ) {
    return d->methods;
}

rstat reconfigure( dict *d, dict_settings settings, size_t max_threads ) {
    uint8_t e = join_epoch( d->prm );
    rstat out = rstat_ok;

    set *s = d->set;
    if ( __sync_bool_compare_and_swap( &(s->rebuild), 0, 1 )) {
        s->settings.max_imbalance = settings.max_imbalance;

        size_t count = settings.slot_count;
        void  *meta  = settings.meta;

        if ( count != s->settings.slot_count || meta != s->settings.meta ) {
            out = do_reconfigure( d, count, meta, max_threads );

            if ( out.bit.error ) {
                __sync_bool_compare_and_swap( &(s->rebuild), 1, 0 );
            }
        }
        else {
            __sync_bool_compare_and_swap( &(s->rebuild), 1, 0 );
        }
    }
    else {
        out = rstat_trans;
    }

    leave_epoch( d->prm, e );
    return out;
}

rstat do_reconfigure( dict *d, size_t slot_count, void *meta, size_t max_threads ) {
    set *s = d->set;

    dict_settings settings = d->set->settings;
    settings.max_imbalance = 0;
    if ( slot_count ) settings.slot_count = slot_count;
    if ( meta )       settings.meta       = meta;

    dict *new_dict = NULL;
    rstat out = do_create( &new_dict, settings, d->methods );
    if ( out.bit.error ) return out;

    out = rstat_ok;

    merge_settings ms = { MERGE_INSERT, 2 };
    out = do_merge( d, new_dict, ms, max_threads, RBLD );

    if ( !out.bit.error ) {
        new_dict->set->immutable = s->immutable;
        dev_assert_or_do( __sync_bool_compare_and_swap( &(d->set), s, new_dict->set ));
        d->set->settings.max_imbalance = s->settings.max_imbalance;
        rebalance_all( d, max_threads );
        dispose( d->prm, s, free_set, d );
        new_dict->set = NULL;
    }

    // Unblock trees if there was an error
    if ( out.bit.error ) {
        rstat cleanup = do_null_swap( s, 0, s->settings.slot_count, RBLD, NULL, max_threads );
        if ( cleanup.bit.error ) {
            // Cannot let this fail.
            for ( size_t i = 0; i < s->settings.slot_count; i++ ) {
                slot *sl = s->slots[i];
                if ( !sl ) continue;
                node *root = sl->root;
                if ( !root ) continue;
                reconf_unblock( root );
            }
        }
    }

    do_free( &new_dict );

    return out;
}

void reconf_unblock( node *n ) {
    __sync_bool_compare_and_swap( &(n->right), RBLD, NULL );
    if ( n->right != NULL ) reconf_unblock( n->right );

    __sync_bool_compare_and_swap( &(n->usref->sref), RBLD, NULL );

    __sync_bool_compare_and_swap( &(n->left), RBLD, NULL );
    if ( n->left != NULL ) reconf_unblock( n->left );
}

