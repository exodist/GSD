#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "devtools.h"
#include "operations.h"
#include "structure.h"
#include "balance.h"
#include "alloc.h"
#include "util.h"

rstat op_trigger( dict *d, void *key, dict_trigger *t, void *targ, void *val ) {
    location *locator = NULL;
    set_spec sp = { 1, 0, NULL, NULL, t, targ };
    rstat err = do_set( d, &locator, key, val, &sp );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

rstat op_get( dict *d, void *key, void **val ) {
    location *loc = NULL;
    rstat err = locate_key( d, key, &loc );

    if ( !err.num ) {
        if ( loc->xtrn == NULL ) {
            *val = NULL;
        }
        else {
            *val = loc->xtrn->value;
            if ( d->methods.ref )
                d->methods.ref( d, *val, 1 );
        }
    }

    // Free our locator
    if ( loc != NULL ) free_location( d, loc );

    return err;
}

rstat op_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    set_spec sp = { 1, 1, NULL, NULL };
    rstat err = do_set( d, &locator, key, val, &sp );

    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

rstat op_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    set_spec sp = { 1, 0, NULL, NULL };
    rstat err = do_set( d, &locator, key, val, &sp );
    if ( locator != NULL ) {
        free_location( d, locator );
    }
    return err;
}

rstat op_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    set_spec sp = { 0, 1, NULL, NULL };
    rstat err = do_set( d, &locator, key, val, &sp );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_delete( dict *d, void *key ) {
    location *locator = NULL;
    set_spec sp = { 0, 1, NULL, NULL };
    rstat err = do_set( d, &locator, key, NULL, &sp );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return error( 1, 0, DICT_API_MISUSE, "NULL can not be used as the 'old value' in compare and swap", 0 );
    location *locator = NULL;
    set_spec sp = { 0, 1, old_val, NULL };
    rstat err = do_set( d, &locator, key, new_val, &sp );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    set_spec sp = { 0, 1, old_val, NULL };
    rstat err = do_set( d, &locator, key, NULL, &sp );
    if ( locator != NULL ) free_location( d, locator );
    return err;
}

rstat op_reference( dict *orig, void *okey, set_spec *osp, dict *dest, void *dkey, set_spec *dsp ) {
    rstat out = rstat_ok;
    dev_assert( dsp->swap_from == NULL );
    location *oloc = NULL;
    location *dloc = NULL;

    // Find existing pairs
    out = locate_key( orig, okey, &oloc );
    if ( out.bit.error ) goto OP_REFERENCE_CLEANUP;
    out = locate_key( dest, dkey, &dloc );
    if ( out.bit.error ) goto OP_REFERENCE_CLEANUP;

    if ( oloc->set->immutable || dloc->set->immutable ) {
        out = rstat_imute;
        goto OP_REFERENCE_CLEANUP;
    }

    // No current value, and cannot insert
    if (( !oloc->sref || !oloc->xtrn ) && !osp->insert ) {
        out = rstat_trans;
        goto OP_REFERENCE_CLEANUP;
    }
    if (( !dloc->sref || !dloc->xtrn ) && !dsp->insert ) {
        out = rstat_trans;
        goto OP_REFERENCE_CLEANUP;
    }

    // Current value, but cannot update
    if ( dloc->xtrn && !dsp->update ) {
        out = rstat_trans;
        goto OP_REFERENCE_CLEANUP;
    }

    if ( !oloc->sref ) {
        out = do_set( orig, &oloc, okey, NULL, osp );

        // Error on all but rebalance issues.
        if ( out.bit.error && !out.bit.rebal )
            goto OP_REFERENCE_CLEANUP;
    }

    if ( !dloc->usref ) {
        out = do_set( dest, &dloc, dkey, NULL, dsp );

        // Error on all but rebalance issues.
        if ( out.bit.error && !out.bit.rebal )
            goto OP_REFERENCE_CLEANUP;
    }

    dev_assert( oloc->sref );
    dev_assert( dloc->usref );

    dev_assert( !blocked_null( oloc->sref ));
    dev_assert( !blocked_null( dloc->usref ));

    out = do_deref( dest, dkey, dloc, oloc->usref->sref );

    OP_REFERENCE_CLEANUP:

    if ( oloc != NULL ) free_location( orig, oloc );
    if ( dloc != NULL ) free_location( dest, dloc );

    return out;
}

rstat op_dereference( dict *d, void *key ) {
    location *loc = NULL;
    rstat err = locate_key( d, key, &loc );

    if ( !err.num && loc->sref != NULL ) err = do_deref( d, key, loc, NULL );

    if ( loc != NULL ) free_location( d, loc );
    return err;
}

rstat do_deref( dict *d, void *key, location *loc, sref *swap ) {
    if ( loc->set->immutable ) return rstat_imute;

    sref *r = loc->sref;
    if ( r == NULL ) return rstat_trans;

    if ( swap != NULL ) {
        int success = 0;
        while ( !success ) {
            size_t sc = swap->refcount;

            // If ref count goes to zero we cannot use it.
            if ( sc == 0 ) return error( 1, 0, DICT_UNKNOWN, "Blah oops", 0 );

            success = __sync_bool_compare_and_swap( &(swap->refcount), sc, sc + 1 );
        }
    }

    // Swap old sref with new sref, skip if sref has changed already
    if( __sync_bool_compare_and_swap( &(loc->usref->sref), r, swap )) {
        if ( d->methods.change ) {
            xtrn *nv = r    ? r->xtrn    : NULL;
            xtrn *ov = swap ? swap->xtrn : NULL;
            d->methods.change( d, loc->set->settings.meta, key,
                nv ? nv->value : NULL,
                ov ? ov->value : NULL
            );
        }

        // Lower ref count of old sref, dispose of sref if count hits 0
        size_t count = __sync_sub_and_fetch( &(r->refcount), 1 );
        if ( count == 0 ) dispose( d, (trash *)r );
        return rstat_ok;
    }
    else {
        size_t count = __sync_sub_and_fetch( &(swap->refcount), 1 );
        if ( count == 0 ) dispose( d, (trash *)swap );
        return rstat_trans;
    }
}

rstat do_set( dict *d, location **locator, void *key, void *val, set_spec *spec ) {
    void *old_val = NULL;
    int retry = 0;
    rstat stat = rstat_ok;

    while ( 1 ) {
        stat = locate_key( d, key, locator );
        if ( stat.num ) return stat;
        location *loc = *locator;
        if ( loc->set->immutable ) return rstat_imute;

        // Check for an existing sref, updating srefs is not blocked by a
        // rebuild.
        if ( loc->sref && !blocked_null( loc->sref )) {
            // If we get an sref we either update it, or don't. No need to move
            // on to other steps. It always returns 0 (do not retry).
            do_set_sref( d, loc, key, val, spec, &old_val, &stat );
            break;
        }

        // Fail transaction if spec does not let us insert
        if ( !spec->insert ) return rstat_trans;

        // Yield if we need to wait for a rebuild. Only check this after the
        // first try.
        if ( retry && (loc->slot->rebuild || loc->set->rebuild )) {
            sleep( 0 );
            continue;
        }

        // Check for existing usref
        if ( loc->usref ) {
            retry = do_set_usref( d, loc, key, val, spec, &stat );
            if ( retry ) continue;
            break;
        }

        // Check for parent node
        if ( loc->parent ) {
            retry = do_set_parent( d, loc, key, val, spec, &stat );
            if ( retry ) continue;
            break;
        }

        dev_assert( loc->slotn_set );
        retry = do_set_slot( d, loc, key, val, spec, &stat );
        if ( retry ) continue;
        break;
    }

    location *loc = *locator;

    if ( loc && d->methods.change && !stat.bit.fail ) {
        d->methods.change( d, loc->set->settings.meta, key, old_val, val );
    }

    return stat;
}

int do_set_sref( dict *d, location *loc, void *key, void *val, set_spec *spec, void **old_val, rstat *stat ) {
    dev_assert( !spec->usref   );
    dev_assert( !spec->trigger );

    if ( loc->sref->trigger ) {
        const char *error = loc->sref->trigger->function(
            loc->sref->trigger->arg->value,
            val
        );

        if ( error ) {
            *stat = rstat_trigg;
            stat->bit.message = error;
            return 0;
        }
    }

    xtrn *new_xtrn = NULL;

    if ( val ) {
        new_xtrn = do_set_create( d, loc->epoch, key, val, CREATE_XTRN, spec );
        if ( !new_xtrn ) {
            *stat = rstat_mem;
            return 0;
        }
    }

    // If this an atomic swap set
    if ( spec->swap_from ) {
        xtrn *current = loc->sref->xtrn;
        if ( !d->methods.cmp( loc->set->settings.meta, current->value, spec->swap_from )) {
            if ( __sync_bool_compare_and_swap( &(loc->sref->xtrn), current, new_xtrn )) {
                *old_val = current->value;
                dispose( d, (trash *)current );
                *stat = rstat_ok;
                return 0;
            }
        }

        dispose( d, (trash *)new_xtrn );
        *stat = rstat_trans;
        return 0;
    }

    while (1) {
        xtrn *current = loc->sref->xtrn;

        // If it is null we need to be able to insert
        // If it is not null we need to be able to update
        if (( !current && !spec->insert ) || ( current && !spec->update )) {
            dispose( d, (trash *)new_xtrn );
            *stat = rstat_trans;
            return 0;
        }

        if ( __sync_bool_compare_and_swap( &(loc->sref->xtrn), current, new_xtrn )) {
            if ( d->methods.change ) {
                d->methods.change(
                    d, loc->set->settings.meta,
                    key,
                    current ? current->value : NULL, val
                );
            }

            if ( current ) dispose( d, (trash *)current );
            *stat = rstat_ok;
            return 0;
        }
    }

    return 0;
}

int do_set_usref( dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat ) {
    dev_assert( !spec->usref );
    sref *new_sref = do_set_create( d, loc->epoch, key, val, CREATE_SREF, spec );
    if( !new_sref ) {
        *stat = rstat_mem;
        return 0;
    }

    if ( __sync_bool_compare_and_swap( &(loc->usref->sref), NULL, new_sref )) {
        loc->sref = new_sref;
        *stat = rstat_ok;
        return 0;
    }

    // Something else resurrected the node, tell do_set to try again.
    dispose( d, (trash *)new_sref );
    return 1;
}

int do_set_parent( dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat ) {
    node *new_node = do_set_create( d, loc->epoch, key, val, CREATE_NODE, spec );
    if ( new_node == NULL ) {
        *stat = rstat_mem;
        return 0; //do not try again
    }

    while ( 1 ) {
        node * volatile *branch = NULL;
        int dir = d->methods.cmp( loc->set->settings.meta, key, loc->parent->key->value );
        switch( dir ) {
            case -1:
                branch = &(loc->parent->left);
            break;
            case 1:
                branch = &(loc->parent->right);
            break;
            case 0:
                dispose( d, (trash *)new_node );
                *stat = error( 1, 0, DICT_UNKNOWN, "This should not be possible unless a nodes key has changed, which is not permitted.", 0 );
                return 0;
            break;
            default:
                dispose( d, (trash *)new_node );
                *stat = error( 1, 0, DICT_API_MISUSE, "The Compare method must return 1, 0, or -1", 0 );
                return 0;
            break;
        }

        // Insert the new node!
        if ( __sync_bool_compare_and_swap( branch, NULL, new_node )) {
            size_t count = __sync_add_and_fetch( &(loc->slot->item_count), 1 );
            __sync_add_and_fetch( &(d->item_count), 1 );

            loc->node  = new_node;
            loc->usref = loc->node->usref;
            loc->sref  = loc->usref->sref;
            if ( blocked_null( loc->sref )) loc->sref = NULL;
            loc->xtrn  = loc->sref ? loc->sref->xtrn : NULL;

            loc->height++;

            *stat = balance_check( d, loc, count );

            return 0;
        }

        // Prepare to try again...
        *stat = locate_key( d, key, &loc );
        if ( stat->num ) {
            dispose( d, (trash *)new_node );
            return 0;
        }

        // Matching node has been inserted :-( all that work for nothing,
        // retry...
        if ( loc->node ) {
            dispose( d, (trash *)new_node );
            return 1;
        }
    }
}

int do_set_slot( dict *d, location *loc, void *key, void *val, set_spec *spec, rstat *stat ) {
    slot *new_slot = do_set_create( d, loc->epoch, key, val, CREATE_SLOT, spec );
    if ( new_slot == NULL ) {
        *stat = rstat_mem;
        return 0; //do not try again
    }

    int success = __sync_bool_compare_and_swap(
        &(loc->set->slots[loc->slotn]),
        NULL,
        new_slot
    );

    // Slot was created by another thread, we need to start over (retry)
    if ( !success ) {
        dispose( d, (trash *)new_slot );
        return 1;
    }

    loc->slot  = new_slot;
    loc->node  = loc->slot->root;
    loc->usref = loc->node->usref;
    loc->sref  = loc->usref->sref;
    if ( blocked_null( loc->sref )) loc->sref = NULL;
    loc->xtrn  = loc->sref ? loc->sref->xtrn : NULL;

    return 0;
}

void *do_set_create( dict *d, epoch *e, void *key, void *val, create_type type, set_spec *spec ) {
    usref *new_usref = NULL;
    if ( spec->usref ) {
        dev_assert( type != CREATE_XTRN );
        dev_assert( type != CREATE_SREF );
        new_usref = spec->usref;
    }
    else {
        xtrn *new_xtrn = NULL;
        if ( val || type == CREATE_XTRN ) {
            dev_assert( val );
            new_xtrn = create_xtrn( d, val );
            if ( !new_xtrn ) return NULL;
        }

        if ( type == CREATE_XTRN ) return new_xtrn;

        trigger_ref *trig = NULL;
        if ( spec->trigger ) {
            trig = malloc( sizeof( trigger_ref ));
            if ( trig == NULL ) {
                if( new_xtrn ) dispose( d, (trash *)new_xtrn );
                return NULL;
            }

            xtrn *arg = create_xtrn( d, spec->trigger_arg );
            if ( !arg ) {
                free( trig );
                if( new_xtrn ) dispose( d, (trash *)new_xtrn );
                return NULL;
            }

            trig->function = spec->trigger;
            trig->arg      = arg;
        }

        sref *new_sref = create_sref( new_xtrn, trig );
        if( !new_sref ) {
            if ( trig ) free( trig );
            if( new_xtrn ) dispose( d, (trash *)new_xtrn );
            return NULL;
        }
        if ( type == CREATE_SREF ) {
            new_sref->refcount = 1;
            return new_sref;
        }

        new_usref = create_usref( new_sref );
        if( !new_usref ) {
            dispose( d, (trash *)new_sref );
            return NULL;
        }
    }

    xtrn *new_xtrn_key = create_xtrn( d, key );
    if ( !new_xtrn_key ) {
        dispose( d, (trash *)new_usref );
        return NULL;
    }

    node *new_node = create_node( new_xtrn_key, new_usref, 0 );
    if ( !new_node ) {
        dispose( d, (trash *)new_usref );
        dispose( d, (trash *)new_xtrn_key );
        return NULL;
    }
    if ( type == CREATE_NODE ) return new_node;

    slot *new_slot = create_slot( new_node );
    if ( !new_slot ) {
        dispose( d, (trash *)new_node );
        return NULL;
    }
    return new_slot;
}

