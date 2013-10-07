#ifndef GSD_DICT_LIST_H
#define GSD_DICT_LIST_H
#include "gsd_dict.h"

typedef struct list list;

typedef void (list_ref)( list *l, void *ref, int delta );

typedef void *(list_pair)( void *key, void *val, void *error );

typedef void (list_unpair)( void *in, void **key, void **val, void *error );

size_t list_size( list *l, size_t length );

list *list_build( uint8_t increments, list_ref *ref );
list *list_combine( list *l1, list *l2 );
list *list_slice( list *l, size_t start, size_t end );

list *dict_keys_to_list  ( dict *d );
list *dict_values_to_list( dict *d );

list *dict_key_refs_to_list  ( dict *d );
list *dict_value_refs_to_list( dict *d );

list *dict_to_list( dict *d, list_pair   *pair,   void *error );
dict *list_to_dict( list *l, list_unpair *unpair, void *error );

dict_stat list_assign( list *from, list *to );

dict_stat list_update( list *l, size_t index, void  *val );
dict_stat list_push  ( list *l, size_t index, void  *val );
dict_stat list_get   ( list *l, size_t index, void **val );

dict_stat list_cmp_update( list *l, size_t index, void *oldval, void *newval );

// These are both forms of push
dict_stat list_reference     ( list *orig, size_t oidx, list *dest, size_t didx );
dict_stat list_reference_dict( dict *orig, void  *okey, list *dest, size_t didx );

#endif
