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

#include "structure.h"

typedef struct location location;

struct location {
    epoch  *epoch;
    set    *set;        // st
    size_t  slotn;      // sltn
    uint8_t slotn_set;  // sltns
    slot   *slot;       // slt
    size_t  height;
    node   *parent;
    node   *node;       // found
    usref  *usref;      // itemp
    sref   *sref;       // item
};

location *dict_create_location( dict *d );
int dict_locate( dict *d, void *key, location **locate );
void dict_free_location( dict *d, location *locate );

#endif
