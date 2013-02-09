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

typedef struct dict  dict;
typedef struct epoch epoch;
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

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args );


#endif
