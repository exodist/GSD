/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef EPOCH_H
#define EPOCH_H

#include <stdlib.h>

typedef struct dict  dict;

typedef struct epoch epoch;
struct epoch {
    /*\
     * epoch count of 0 means free
     * epoch count of 1 means needs to be cleared (not joinable)
     * epoch count of >1 means active
    \*/
    size_t active;
    void  *garbage;
    void  *meta;
    enum  { SET, SLOT, NODE, SREF } gtype;

    epoch *dep;
    epoch *next;
};

epoch *create_epoch();
void dispose( dict *d, epoch *e, void *meta, void *garbage, int type );
epoch *join_epoch( dict *d );
void leave_epoch( dict *d, epoch *e );

#endif
