#ifndef GSD_DICT_H
#define GSD_DICT_H

#include <stdint.h>
#include <stdlib.h>

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
typedef void   (dict_hook)( dict *d, void *meta, void *key );
typedef int    (dict_handler)( void *item, void *args );
typedef int    (dict_cmp)( void *meta, void *key1, void *key2 );
typedef size_t (dict_loc)( void *meta, size_t slot_count, void *key );
typedef void   (dict_del)( void *del );

struct dict_methods {
    dict_cmp *cmp;  // Used to compare keys ( -1, 0, 1 : left, same, right )
    dict_loc *loc;  // Used to find slot of key
    dict_del *del;  // Used to free internal objects
    dict_del *rem;  // Used to free external objects

    dict_hook *ins; // Callback when a new key is inserted
    dict_hook *dlt; // Callback when a key is removed
};

struct dict;

// -- Creation and meta data --

#define dict_create( a, b, c, d, e ) x_dict_create( a, b, c, d, e, __FILE__, __LINE__ )
int x_dict_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods, char *file, size_t line );

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

int dict_reference( dict *orig, dict *dest, dict *key );

int dict_iterate( dict *d, dict_handler *h, void *args );

#endif
