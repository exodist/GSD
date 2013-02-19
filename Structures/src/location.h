/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef LOCATION_H
#define LOCATION_H

#include "error.h"
#include "structure.h"

typedef struct location location;

struct location {
    epoch   *epoch;
    set     *set;
    size_t   slotn;
    uint8_t  slotn_set;
    slot    *slot;
    size_t   height;
    node    *parent;
    int8_t  *dir;
    node    *node;
    usref   *usref;
    sref    *sref;
};

location *create_location( dict *d );
rstat locate_key( dict *d, void *key, location **locate );
void free_location( dict *d, location *locate );
rstat locate_from_node( dict *d, void *key, location **locate, node *n );

#endif
