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
typedef struct node  node;
typedef struct flags flags;
typedef struct usref usref;
typedef struct sref  sref;

struct dict {
    set *set;

    dict_methods *methods;

    epoch *epochs;
    epoch *epoch;
    uint8_t epoch_limit;
    uint8_t epoch_count;
};

struct set {
    slot **slots;
    dict_settings *settings;
};

struct slot {
    node   *root;
    size_t  count;
    uint8_t ideal_height;
    uint8_t rebuild;
};

struct node {
    node  *left;
    node  *right;
    void  *key;
    usref *usref;
};

struct usref {
    uint8_t refcount;
    sref   *sref;
};

struct sref {
    size_t  refcount;
    void   *value;
};

int iterate( dict *d, dict_handler *h, void *args );

int iterate_node( dict *d, node *n, dict_handler *h, void *args );


#endif

