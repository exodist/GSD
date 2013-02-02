#include "gsd_dict_api.h"
#include "gsd_dict_balance.h"
#include "gsd_dict_dot.h"
#include "gsd_dict_epoch.h"
#include "gsd_dict_free.h"
#include "gsd_dict_location.h"
#include "gsd_dict_structures.h"
#include "gsd_dict_util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// -- Creation and meta data --

dict_settings *dict_get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods *dict_get_methods( dict *d ) {
    return d->methods;
}

int dict_free( dict **dr ) {
    dict *d = *dr;
    *dr = NULL;

    // Wait on all epochs
    int active = 1;
    while ( active ) {
        active = 0;
        for ( int i = 0; i < 10; i++ ) {
            epoch *e = d->epochs[i];
            active += e->active;
        }
        if ( active ) sleep( 0 );
    }

    if ( d->set != NULL ) dict_free_set( d, d->set );
    free( d );

    return DICT_NO_ERROR;
}

int dict_create_vb( dict **d, uint8_t c, dict_settings *s, dict_methods *m, char *f, size_t l ) {
    if ( s == NULL ) {
        fprintf( stderr, "Settings may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }
    if ( m == NULL ) {
        fprintf( stderr, "Methods may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }
    if ( m->cmp == NULL ) {
        fprintf( stderr, "The 'cmp' method may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }
    if ( m->loc == NULL ) {
        fprintf( stderr, "The 'loc' method may not be NULL. Called from %s line %zi", f, l );
        return DICT_API_ERROR;
    }

    return dict_do_create( d, c, s, m );
}

int dict_create( dict **d, uint8_t epoch_count, dict_settings *settings, dict_methods *methods ) {
    if ( settings == NULL ) return DICT_API_ERROR;
    if ( methods == NULL )      return DICT_API_ERROR;
    if ( methods->cmp == NULL ) return DICT_API_ERROR;
    if ( methods->loc == NULL ) return DICT_API_ERROR;

    return dict_do_create( d, epoch_count, settings, methods );
}

// Copying and cloning
int dict_merge( dict *from, dict *to );
int dict_merge_refs( dict *from, dict *to );

// -- Informational --

int dict_dump_dot( dict *d, char **buffer, dict_dot *show ) {
    epoch *e = NULL;
    dict_join_epoch( d, NULL, &e );
    set *s = d->set;

    dot dt = { NULL, 0, show, NULL };

    int error = dict_dump_dot_start( &dt );
    if( error ) {
        dict_leave_epoch( d, e );
        return error;
    }

    int last = -1;
    for ( int i = 0; i < s->settings->slot_count; i++ ) {
        slot *sl = s->slots[i];
        if ( sl == NULL ) continue;
        error = dict_dump_dot_slink( &dt, last, i );
        if ( error ) break;
        last = i;

        node *n = sl->root;
        if ( n == NULL ) continue;
        error = dict_dump_dot_subgraph( &dt, i, n );
        if ( error ) break;
    }

    if ( !error ) {
        error = dict_dump_dot_write( &dt, "}" );
    }

    dict_leave_epoch( d, e );

    *buffer = dt.buffer;

    return error;
}

// -- Operation --

// Chance to handle pathological data gracefully
int dict_reconfigure( dict *d, dict_settings *settings ) {
    return DICT_UNIMP_ERROR;
}

int dict_get( dict *d, void *key, void **val ) {
    location *loc = NULL;
    int err = dict_locate( d, key, &loc );

    if ( !err ) {
        if ( loc->sref == NULL ) {
            *val = NULL;
        }
        else {
            *val = loc->sref->value;
        }
    }

    // Free our locator
    if ( loc != NULL ) dict_free_location( d, loc );

    return err;
}

int dict_set( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 1, &locator );
    if ( locator != NULL ) {
        dict_free_location( d, locator );
    }
    return err;
}

int dict_insert( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 0, 1, &locator );
    if ( locator != NULL ) {
        dict_free_location( d, locator );
    }
    return err;
}

int dict_update( dict *d, void *key, void *val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, val, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_delete( dict *d, void *key ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, NULL, NULL, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val ) {
    if ( old_val == NULL ) return DICT_API_ERROR;
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, new_val, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_cmp_delete( dict *d, void *key, void *old_val ) {
    location *locator = NULL;
    int err = dict_do_set( d, key, old_val, NULL, 1, 0, &locator );
    if ( locator != NULL ) dict_free_location( d, locator );
    return err;
}

int dict_reference( dict *orig, void *okey, dict *dest, void *dkey ) {
    location *oloc = NULL;
    location *dloc = NULL;

    // Find item in orig, insert if necessary
    int err1 = dict_do_set( orig, okey, NULL, NULL, 0, 1, &oloc );
    // Find item in dest, insert if necessary
    int err2 = dict_do_set( dest, dkey, NULL, NULL, 0, 1, &dloc );

    // Ignore rebalance errors.. might want to readdress this.
    if ( err1 > 100 ) err1 = 0;
    if ( err2 > 100 ) err2 = 0;

    // Transaction error from above simply means it already exists
    if ( err1 == DICT_TRANS_FAIL ) err1 = 0;
    if ( err2 == DICT_TRANS_FAIL ) err2 = 0;

    if ( !err1 && !err2 ) {
        dict_do_deref( dest, dkey, dloc, oloc->usref->sref );
    }

    if ( oloc != NULL ) dict_free_location( orig, oloc );
    if ( dloc != NULL ) dict_free_location( dest, dloc );

    if ( err1 ) return err1;
    if ( err2 ) return err2;

    return DICT_NO_ERROR;
}

int dict_dereference( dict *d, void *key ) {
    location *loc = NULL;
    int err = dict_locate( d, key, &loc );

    if ( !err && loc->sref != NULL ) dict_do_deref( d, key, loc, NULL );

    if ( loc != NULL ) dict_free_location( d, loc );
    return err;
}

int dict_iterate( dict *d, dict_handler *h, void *args ) {
    epoch *e = NULL;
    dict_join_epoch( d, NULL, &e );
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


