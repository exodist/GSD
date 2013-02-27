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
        s->settings.max_internal_threads = settings.max_internal_threads;

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
    rstat out = do_create( &new_dict, d->epoch_limit, settings, d->methods );
    if ( out.bit.error ) return out;

    size_t index = 0;
    void *args[4] = { &index, s, NULL, RBLD };

    out = rstat_ok;
    if ( max_threads < 2 ) {
        rstat *ret = reconf_worker( args );
        out = *ret;
        // Note: If return is an error then it will be dynamically
        // allocated memory we need to free, unless there was a
        // memory error in which case we will have a ref to
        // rstat_mem which we should not free.
        if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
    }
    else {
        pthread_t *pts = malloc( max_threads * sizeof( pthread_t ));
        if ( !pts ) {
            return rstat_mem;
        }
        else {
            for ( int i = 0; i < max_threads; i++ ) {
                pthread_create( &(pts[i]), NULL, reconf_worker, args );
            }
            for ( int i = 0; i < max_threads; i++ ) {
                rstat *ret;
                pthread_join( pts[i], (void **)&ret );
                if ( ret->bit.error && !out.bit.error ) {
                    out = *ret;
                    // Note: If return is an error then it will be dynamically
                    // allocated memory we need to free, unless there was a
                    // memory error in which case we will have a ref to
                    // rstat_mem which we should not free.
                    if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
                }
            }
            free( pts );
        }
    }

    merge_settings ms = { MERGE_INSERT, 1 };
    out = do_merge( d, new_dict, ms, max_threads );

    if ( !out.bit.error ) {
        fprintf( stderr, "\n[%zi -> %zi]\n", d->item_count, new_dict->item_count );
        fflush( stderr );
        assert( d->item_count == new_dict->item_count );
        assert( __sync_bool_compare_and_swap( &(d->set), s, new_dict->set ));
        d->set->settings.max_imbalance = s->settings.max_imbalance;
        rebalance_all( d, 2, max_threads );
        dispose( d, (trash *)s );
        new_dict->set = NULL;
    }

    // Unblock slots
    if ( out.bit.error ) {
        index = 0;
        args[2] = RBLD;
        args[3] = NULL;
    
        if ( max_threads < 2 ) {
            rstat *ret = reconf_worker( args );
            out = *ret;
            // Note: If return is an error then it will be dynamically
            // allocated memory we need to free, unless there was a
            // memory error in which case we will have a ref to
            // rstat_mem which we should not free.
            if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
        }
        else {
            pthread_t *pts = malloc( max_threads * sizeof( pthread_t ));
            if ( !pts ) {
                return rstat_mem;
            }
            else {
                for ( int i = 0; i < max_threads; i++ ) {
                    pthread_create( &(pts[i]), NULL, reconf_worker, args );
                }
                for ( int i = 0; i < max_threads; i++ ) {
                    rstat *ret;
                    pthread_join( pts[i], (void **)&ret );
                    if ( ret->bit.error && !out.bit.error ) {
                        out = *ret;
                        // Note: If return is an error then it will be dynamically
                        // allocated memory we need to free, unless there was a
                        // memory error in which case we will have a ref to
                        // rstat_mem which we should not free.
                        if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
                    }
                }
                free( pts );
            }
        }
    }

    do_free( &new_dict );

    return out;
}

void *reconf_worker( void *in ) {
    void **args = (void **)in;
    size_t *index = args[0];
    set    *set   = args[1];
    void   *from  = args[2];
    void   *to    = args[3];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= set->settings.slot_count ) return NULL;

        rstat check = reconf_prep_slot( set, idx, from, to );
        if ( check.bit.error ) {
            // Allocate a new return, or return a ref to rstat_mem.
            rstat *out = malloc( sizeof( rstat ));
            if ( out == NULL ) return &rstat_mem;
            *out = check;
            return out;
        }
    }
}

rstat reconf_prep_slot( set *set, size_t idx, void *from, void *to ) {
    rstat out = rstat_ok;
    slot *sl = set->slots[idx];
    if ( sl == NULL ) return out;

    nlist *nodes = nlist_create();
    if ( !nodes ) return rstat_mem;

    node *n = sl->root;
    while( n != NULL ) {
        __sync_bool_compare_and_swap( &(n->right), from, to );
        if ( n->right != RBLD && n->right != NULL ) {
            out = nlist_push( nodes, n->right );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        __sync_bool_compare_and_swap( &(n->left), from, to );
        if ( n->left != RBLD && n->left != NULL ) {
            out = nlist_push( nodes, n->left );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        __sync_bool_compare_and_swap( &(n->usref->sref), from, to );

        n = nlist_shift( nodes );
    }

    MERGE_XFER_ERROR:
    nlist_free( &nodes );
    return out;
}
