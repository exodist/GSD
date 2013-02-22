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

typedef struct trash trash;
typedef struct dict  dict;
typedef struct set   set;
typedef struct slot  slot;
typedef struct xtrn  xtrn;
typedef struct node  node;
typedef struct flags flags;
typedef struct usref usref;
typedef struct sref  sref;

struct trash {
    trash *next;
    enum { OOPS = 0, SET, SLOT, NODE, SREF, XTRN } type;
    char *fn;
    size_t ln;
};

struct dict {
    set *set;

    dict_methods *methods;

    epoch *epochs;
    epoch *epoch;
    size_t epoch_limit;
    size_t epoch_count;

    size_t rebalanced;
};

struct set {
    trash  trash;
    slot **slots;
    dict_settings *settings;
    uint8_t rebuild;
};

struct slot {
    trash   trash;
    node   *root;
    size_t  count;
    uint8_t ideal_height;
    uint8_t rebuild;
    uint8_t patho;
};

struct xtrn {
    trash trash;
    void *value;
};

struct node {
    trash trash;
    node  *left;
    node  *right;
    xtrn  *key;
    usref *usref;
};

struct usref {
    size_t  refcount;
    sref   *sref;
};

struct sref {
    trash   trash;
    size_t  refcount;
    xtrn   *xtrn;
};

int iterate( dict *d, dict_handler *h, void *args );

int iterate_node( dict *d, node *n, dict_handler *h, void *args );


#endif

