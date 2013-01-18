#ifndef GSD_DICT_API_H
#define GSD_DICT_API_H

#include <stdint.h>
#include <stdlib.h>

#define DICT_FATAL_ERROR -5
#define DICT_SLOT_ERROR  -4
#define DICT_CMP_ERROR   -3
#define DICT_UNIMP_ERROR -2
#define DICT_PATHO_ERROR -1
#define DICT_NO_ERROR     0
#define DICT_MEM_ERROR    1
#define DICT_EXIST_ERROR  2
#define DICT_TRANS_ERROR  3
#define DICT_API_ERROR    4

typedef struct dict_methods dict_methods;
typedef struct dict dict;
typedef void   (dict_hook)( dict *d, void *meta, void *key, void *val );
typedef int    (dict_handler)( void *item, void *args );
typedef int    (dict_cmp)( void *meta, void *key1, void *key2 );
typedef size_t (dict_loc)( void *meta, size_t slot_count, void *key );

struct dict;

struct dict_methods {
    dict_cmp *cmp;  // Used to compare keys ( -1, 0, 1 : left, same, right )
    dict_loc *loc;  // Used to find slot of key

    dict_hook *ins; // Callback when a new pair is inserted
    dict_hook *rem; // Callback when an item is removed (key and/or value)
};

// -- Creation and meta data --

#define dict_create_verbose( a, b, c, d, e ) dict_create_vb( a, b, c, d, e, __FILE__, __LINE__ )
int dict_create_vb( dict **d, size_t s, size_t mi, void *mta, dict_methods *mth, char *f, size_t l );

int dict_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods );

int dict_free( dict **d );

void *dict_meta( dict *d );

// -- Operation --

// Chance to handle pathological data gracefully
int dict_rebuild( dict *d, size_t slots, size_t max_imb, void *meta );

int dict_get( dict *d, void *key, void **val );
int dict_set( dict *d, void *key, void *val );
int dict_insert( dict *d, void *key, void *val );
int dict_update( dict *d, void *key, void *val );
int dict_delete( dict *d, void *key );

int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val );
int dict_cmp_delete( dict *d, void *key, void *old_val );

int dict_reference( dict *orig, void *okey, dict *dest, void *dkey );
int dict_dereference( dict *d, void *key );

int dict_iterate( dict *d, dict_handler *h, void *args );

#endif
