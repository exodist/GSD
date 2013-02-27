#include <pthread.h>

#include "multidict.h"
#include "structure.h"
#include "alloc.h"
#include "operations.h"
#include "node_list.h"
#include "balance.h"

int SUCCESS = 0;
int FAIL = 1;

dict *clone( dict *d, uint8_t reference, size_t threads ) {
    dict *out = NULL;
    rstat ret = do_create( &out, d->epoch_limit, d->set->settings, d->methods );
    if ( !ret.bit.error && out != NULL ) {
        merge_settings s = { MERGE_INSERT, reference };
        ret = do_merge( d, out, s, threads );
    }

    if ( ret.bit.error && out != NULL ) {
        dict_free( &out );
        out = NULL;
    }

    return out;
}

rstat merge( dict *from, dict *to, merge_settings s, size_t threads ) {
    return do_merge( from, to, s, threads );
}

rstat do_merge( dict *orig, dict *dest, merge_settings s, size_t threads ) {
    epoch *eo = join_epoch( orig );
    epoch *ed = join_epoch( dest );
    rstat out = rstat_ok;

    size_t index = 0;
    void *args[5] = {
        &index,
        orig->set,
        orig,
        dest,
        &s
    };

    if ( threads < 2 ) {
        rstat *ret = merge_worker( args );
        out = *ret;
        // Note: If return is an error then it will be dynamically
        // allocated memory we need to free, unless there was a
        // memory error in which case we will have a ref to
        // rstat_mem which we should not free.
        if ( ret != &rstat_mem && ret != &rstat_ok ) free( ret );
    }
    else {
        pthread_t *pts = malloc( threads * sizeof( pthread_t ));
        if ( !pts ) {
            out = rstat_mem;
        }
        else {
            for ( int i = 0; i < threads; i++ ) {
                pthread_create( &(pts[i]), NULL, merge_worker, args );
            }
            for ( int i = 0; i < threads; i++ ) {
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

    leave_epoch( orig, eo );
    leave_epoch( dest, ed );
    return out;
}

void *merge_worker( void *in ) {
    void **args = (void **)in;
    size_t *index = args[0];
    set    *oset  = args[1];
    dict   *orig  = args[2];
    dict   *dest  = args[3];

    merge_settings *settings = args[4];

    while ( 1 ) {
        size_t idx = __sync_fetch_and_add( index, 1 );
        if ( idx >= oset->settings.slot_count ) return (void *)&rstat_ok;

        rstat check = merge_transfer_slot( oset, idx, orig, dest, settings );
        if ( check.bit.error ) {
            // Allocate a new return, or return a ref to rstat_mem.
            rstat *out = malloc( sizeof( rstat ));
            if ( out == NULL ) return &rstat_mem;
            *out = check;
            return out;
        }
    }
}

rstat merge_transfer_slot( set *oset, size_t idx, dict *orig, dict *dest, merge_settings *settings ) {
    rstat out = rstat_ok;
    slot *sl = oset->slots[idx];
    if ( sl == NULL ) return out;

    nlist *nodes = nlist_create();
    if ( !nodes ) return rstat_mem;

    set_spec orig_spec = { 0, 0, NULL };
    set_spec dest_spec = { 0, 0, NULL };
    switch ( settings->operation ) {
        case MERGE_SET:
            dest_spec.insert = 1;
            dest_spec.update = 1;
        break;

        case MERGE_INSERT:
            dest_spec.insert = 1;
        break;

        case MERGE_UPDATE:
            dest_spec.update = 1;
        break;
    };

    node *n = sl->root;
    while( n != NULL ) {
        if ( n->right != RBLD && n->right != NULL ) {
            out = nlist_push( nodes, n->right );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        if ( n->left != RBLD && n->left != NULL ) {
            out = nlist_push( nodes, n->left );
            if ( out.bit.error ) goto MERGE_XFER_ERROR;
        }

        // If this node has no sref we skip it.
        sref *sr = n->usref->sref;
        xtrn *x  = (sr && sr != RBLD) ? sr->xtrn : NULL;
        if ( x != RBLD && x != NULL ) {
            if ( settings->reference ) {
                out = op_reference( orig, n->key->value, &orig_spec, dest, n->key->value, &dest_spec );
                // Failure is not an error, it means the set_specs prevent us
                // from merging this value, which is desired.
                if ( out.bit.error ) goto MERGE_XFER_ERROR;
            }
            else {
                location *loc = NULL;
                out = do_set( dest, &loc, n->key->value, x->value, &dest_spec );
                if ( loc != NULL ) free_location( dest, loc );
                // Failure is not an error, it means the set_specs prevent us
                // from merging this value, which is desired.
                if ( out.bit.error ) goto MERGE_XFER_ERROR;
            }
        }

        n = nlist_shift( nodes );
    }

    MERGE_XFER_ERROR:
    nlist_free( &nodes );
    return out;
}
