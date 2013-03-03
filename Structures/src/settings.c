#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "include/gsd_dict.h"

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
    epoch *e = join_epoch( d );
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

    leave_epoch( d, e );
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
        assert( __sync_bool_compare_and_swap( &(d->set), s, new_dict->set ));
        d->set->settings.max_imbalance = s->settings.max_imbalance;
        rebalance_all( d, max_threads );
        dispose( d, (trash *)s );
        new_dict->set = NULL;
    }

    // Unblock trees if there was an error
    if ( out.bit.error ) {
        rstat cleanup = do_null_swap( s, 0, s->settings.slot_count, RBLD, NULL, max_threads );
        if ( cleanup.bit.error ) out.bit.invalid_state = 1;
    }

    do_free( &new_dict );

    return out;
}

