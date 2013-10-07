/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "epoch.h"
#include "include/gsd_dict.h"
#include "error.h"

typedef struct dict  dict;
typedef struct set   set;
typedef struct slot  slot;
typedef struct xtrn  xtrn;
typedef struct node  node;
typedef struct flags flags;
typedef struct usref usref;
typedef struct sref  sref;
typedef struct trigger_ref trigger_ref;

struct dict {
    dict_methods *methods;

    set * volatile set;

    volatile size_t item_count;

#ifdef GSD_METRICS
    size_t rebalanced;
#endif

    volatile size_t detached_threads;

    epoch_set epochs;
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

struct xtrn {
    void *value;
};

struct node {
    node  * volatile left;
    node  * volatile right;
    xtrn  * key;
    usref * volatile usref;
};

struct usref {
    volatile size_t refcount;
    sref * volatile sref;
};

struct sref {
    volatile size_t refcount;
    xtrn * volatile xtrn;
    trigger_ref *trigger;
};

struct trigger_ref {
    dict_trigger *function;
    xtrn         *arg;
};

int iterate( dict *d, dict_handler *h, void *args );

int iterate_node( dict *d, set *s, node *n, dict_handler *h, void *args );

#endif

