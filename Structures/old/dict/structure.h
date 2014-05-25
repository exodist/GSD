/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef DICT_STRUCTURE_H
#define DICT_STRUCTURE_H

#include "include/gsd_dict.h"
#include "error.h"
#include "../../PRM/src/include/gsd_prm.h"

typedef struct dict  dict;
typedef struct set   set;
typedef struct slot  slot;
typedef struct node  node;
typedef struct flags flags;

struct dict {
    dict_methods methods;

    set * volatile set;

    volatile size_t item_count;

    volatile size_t detached_threads;

    prm *prm;
};

struct set {
    slot ** volatile slots;
    dict_settings settings;

    uint8_t immutable;
    volatile uint8_t rebuild;
};

struct slot {
    node * volatile root;

    volatile size_t  item_count;

    volatile uint8_t ideal_height;
    volatile uint8_t rebuild;
    volatile uint8_t patho;
};

struct node {
    node  * volatile left;
    node  * volatile right;
    void  * key;
    usref * volatile usref;
};

int iterate( dict *d, dict_handler *h, void *args );

int iterate_node( dict *d, set *s, node *n, dict_handler *h, void *args );

#endif

