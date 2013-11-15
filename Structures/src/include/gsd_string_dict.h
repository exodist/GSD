#ifndef GSD_STRING_DICT_H
#define GSD_STRING_DICT_H

#include "gsd_dict.h"
#include "gsd_structures.h"

dict *new_string_dict(size_t slots, void (release)(void *));

get_stat     strd_get( dict *d, uint8_t *key            );
dict_stat    strd_set( dict *d, uint8_t *key, void *val );
dict_stat strd_update( dict *d, uint8_t *key, void *val );
dict_stat strd_insert( dict *d, uint8_t *key, void *val );
dict_stat strd_delete( dict *d, uint8_t *key            );

#endif
