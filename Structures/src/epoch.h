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

#include "error.h"
#include "include/gsd_dict_epoch.h"

#define FORK_FOR_TRASH_COUNT 20 

typedef struct compactor   compactor;
typedef struct epoch       epoch;
typedef struct epoch_set   epoch_set;
typedef struct garbage_bin garbage_bin;
typedef struct trash       trash;

struct trash {
    void       *mem;
    destructor *destroy;
};

struct compactor {
    trash *garbage;
    size_t idx;

    compactor *next;
};

struct epoch {
    /*\
     * epoch count of 0 means free
     * epoch count of 1 means needs to be cleared (not joinable)
     * epoch count of >1 means active
    \*/
    size_t active;
    epoch *dep;
    compactor *compactor;
};

struct epoch_set {
    epoch *epochs;
    uint32_t detached_threads;
    uint8_t  count;
    uint8_t  current;
    size_t   compactor_size;
};

struct garbage_bin {
    epoch_set *set;
    compactor *compactor;
    epoch     *dep;
};


compactor *new_compactor(epoch *e, compactor *old, size_t size);
void *garbage_truck( void *args );
void free_garbage( epoch_set *s, compactor *c );

epoch *advance_epoch( epoch_set *s, epoch *e );

#endif

