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
typedef struct dot   dot;

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
    enum { SET, SLOT, NODE, REF } gtype;
    uint8_t deps[EPOCH_COUNT];
};

struct dict {
    set *set;
    set *rebuild;

    dict_methods *methods;

    dict *cow;
    uint8_t cow_ref;

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
    size_t  node_count;
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
    node   *parent;
    node   *found;
    ref    *item;
    size_t imbalance;

    epoch *epoch;
};

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args );

int dict_do_create( dict **d, size_t slots, size_t max_imb, void *meta, dict_methods *methods );

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

void dict_do_deref( dict *d, void *key, location *loc );

set *create_set( size_t slot_count, void *meta, size_t max_imb );
location *dict_create_location( dict *d );

void dict_free_location( dict *d, location *locate );
void dict_dispose( dict *d, epoch *e, void *garbage, int type );
void dict_join_epoch( dict *d, uint8_t *idx, epoch **ep );
void dict_leave_epoch( dict *d, epoch *e );
int dict_locate( dict *d, void *key, location **locate );

void dict_free_set( set *s );
void dict_free_slot( slot *s );
void dict_free_node( node *n );
void dict_free_ref( ref *r );

int rebalance( dict *d, location *loc );

int dict_dump_dot_start( dot *dt );
int dict_dump_dot_slink( dot *dt, int s1, int s2 );
int dict_dump_dot_subgraph( dot *dt, int s, node *n );
int dict_dump_dot_node( dot *dt, char *line, node *n, char *label );
int dict_dump_dot_write( dot *dt, char *add );

#endif
