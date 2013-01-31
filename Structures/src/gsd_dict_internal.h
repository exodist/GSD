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
typedef struct set   set;
typedef struct epoch epoch;
typedef struct dot   dot;

typedef struct sref  sref;
typedef struct usref usref;

typedef struct location location;

#define EPOCH_COUNT 10
#define DOT_BUFFER_SIZE 256

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

int dict_iterate_node( dict *d, node *n, dict_handler *h, void *args );

int dict_do_create( dict **d, size_t slots, void *meta, dict_methods *methods );

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

void dict_do_deref( dict *d, void *key, location *loc, sref *swap );

set *create_set( size_t slot_count, void *meta );
location *dict_create_location( dict *d );

void dict_free_location( dict *d, location *locate );
void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type );
void dict_join_epoch( dict *d, uint8_t *idx, epoch **ep );
void dict_leave_epoch( dict *d, epoch *e );
int dict_locate( dict *d, void *key, location **locate );

void dict_free_set( dict *d, set *s );
void dict_free_slot( dict *d, void *meta, slot *s );
void dict_free_node( dict *d, void *meta, node *n );
void dict_free_sref( dict *d, sref *r );

int rebalance( dict *d, location *loc );
size_t rebalance_node( node *n, node ***all, size_t *size, size_t count );
int rebalance_insert_list( dict *d, set *st, slot *s, node **all, size_t start, size_t end, size_t ideal );
int rebalance_insert( dict *d, set *st, slot *s, node *n, size_t ideal );

int dict_dump_dot_start( dot *dt );
int dict_dump_dot_slink( dot *dt, int s1, int s2 );
int dict_dump_dot_subgraph( dot *dt, int s, node *n );
int dict_dump_dot_node( dot *dt, char *line, node *n, char *label );
int dict_dump_dot_write( dot *dt, char *add );

size_t tree_ideal_height( size_t count );

#endif
