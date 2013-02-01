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
#include "gsd_dict_api.h"

typedef struct slot  slot;
typedef struct node  node;
typedef struct set   set;
typedef struct epoch epoch;
typedef struct dot   dot;

typedef struct sref  sref;
typedef struct usref usref;

typedef struct location location;

#define EPOCH_COUNT 10

/*\
 * epoch count of 0 means free
 * epoch count of 1 means needs to be cleared (not joinable)
 * epoch count of >1 means active
\*/
struct epoch {
    size_t  active;
    void   *garbage;
    void   *meta;
    enum { SET, SLOT, NODE, SREF } gtype;
    uint8_t deps[EPOCH_COUNT];
};

struct dict {
    set *set;
    set *rebuild;

    dict_methods *methods;

    epoch epochs[EPOCH_COUNT];
    uint8_t epoch;
};

struct set {
    slot  **slots;
    size_t  slot_count;
    void   *meta;
};

struct slot {
    node   *root;
    size_t  count;
    uint8_t rebuild;
};

struct node {
    node  *left;
    node  *right;
    void  *key;
    usref *value;
};

struct sref {
    size_t  refcount;
    void   *value;
};

struct usref {
    uint8_t refcount;
    sref   *value;
};

struct dot {
    char     *buffer;
    size_t    size;
    dict_dot *show;
};

struct location {
    set    *st;
    size_t  sltn;
    uint8_t sltns;
    slot   *slt;
    size_t  height;
    node   *parent;
    node   *found;
    usref  *itemp;
    sref   *item;

    epoch *epoch;
};

#endif
