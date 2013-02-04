/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the gsd_dict_api.h header file in external programs.
\*/

#ifndef GSD_DICT_STRUCTURES_H
#define GSD_DICT_STRUCTURES_H

#include <stdint.h>
#include <stdio.h>
#include "gsd_dict_api.h"

typedef struct slot  slot;
typedef struct node  node;
typedef struct set   set;
typedef struct epoch epoch;
typedef struct dot   dot;
typedef struct flags flags;

typedef struct sref  sref;
typedef struct usref usref;

typedef struct location location;

/*\
 * epoch count of 0 means free
 * epoch count of 1 means needs to be cleared (not joinable)
 * epoch count of >1 means active
\*/
struct epoch {
    size_t active;
    void  *garbage;
    void  *meta;
    enum  { SET, SLOT, NODE, SREF } gtype;

    epoch *dep;
    epoch *next;
};

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

#endif
