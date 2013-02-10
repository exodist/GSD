/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef NODE_LIST_H
#define NODE_LIST_H

#include "structure.h"

/* NOTE: nlist is not concurrent-safe!
 * If you need a node list that is safe for concurrent use, you will have to
 * make variation on push, and shift which use atomic swaps, and use the epoch
 * system to dispose of garbage.
\*/

typedef struct nlist nlist;
typedef struct nlist_item nlist_item;

struct nlist {
    nlist_item *first;
    nlist_item *last;
};

struct nlist_item {
    node *node;
    nlist_item *next;
};

nlist *nlist_create();

int nlist_push( nlist *nl, node *n );

node *nlist_shift( nlist *nl );

void nlist_free( nlist **nl );

#endif
