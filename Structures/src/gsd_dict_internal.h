#ifndef GSD_DICT_INTERNAL_H
#define GSD_DICT_INTERNAL_H

#include "gsd_dict_api.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct slot  slot;
typedef struct node  node;
typedef struct ref   ref;
typedef struct set   set;

typedef struct epoch epoch;
typedef struct epoch_stack epoch_stack;

typedef struct location location;

#define EPOCH_COUNT 10

struct epoch {
    size_t active;
    epoch_stack *stack;
};

struct epoch_stack {
    epoch_stack *down;
    enum { DSLOT, DNODE, DREF, DSET } type;
    void *to_free;
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
    slot  **slot_rebuild;
    size_t  slot_count;
    size_t  max_imbalance;
    void   *meta;
};

struct slot {
    node   *root;
    size_t node_count;
    size_t deleted;
};

struct node {
    node *left;
    node *right;
    void *key;
    ref  *value;
};

struct ref {
    size_t  refcount;
    void   *value;
};

struct location {
    set    *st;
    size_t  sltn;
    uint8_t sltns;
    slot   *slt;
    node   *parent;
    node   *found;
    ref    *item;
    size_t imbalance;

    epoch *epoch;
};

int dict_do_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods );

set *create_set( size_t slot_count, void *meta, size_t max_imb );

void dict_new_epoch( dict *d, epoch *e );
void dict_collect( epoch *e );

int dict_locate( dict *d, void *key, location **locate );
void dict_free_location( location *locate );
location *dict_create_location( dict *d );

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

void dict_do_deref( dict *d, ref **r );

#endif
