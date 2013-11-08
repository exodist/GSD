#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "include/gsd_tokenizer_api.h"
#include "../../Structures/src/include/gsd_dict.h"

typedef struct alt_node   alt_node;
typedef struct dict_entry dict_entry;
typedef struct grammar    grammar;
typedef struct group_node group_node;
typedef struct name_node  name_node;
typedef struct node       node;
typedef struct prim_node  prim_node;
typedef struct scope_node scope_node;
typedef struct slurp_node slurp_node;

struct grammar {
    dict *patterns;

    keyword_check *kc;
    void          *key_arg;
    keyword_run   *kr;
};

struct dict_entry {
    size_t ref_count;
    enum { PATTERN, GRAMMAR } type;
};

struct node {
    dict_entry entry;
    enum {
        NODE_ALT,
        NODE_GROUP,
        NODE_NAME,
        NODE_PRIM,
        NODE_SCOPE,
        NODE_SLURP,
    } type;

    enum { Q_ONE, Q_OPTIONAL, Q_ANY, Q_SOME } quantifier;

    uint8_t no_capture;

    node *next;
};

struct prim_node {
    node node;
    enum {
        PRIM_ALPHA,
        PRIM_DIGIT,
        PRIM_ALPHADIGIT,
        PRIM_QUOTE,
        PRIM_SPACE,
        PRIM_NOSPACE,
        PRIM_SYMBOL,
        PRIM_CONTROL,
    } match;
};

struct name_node {
    node node;
    uint8_t *name;
    node *pattern;
};

struct group_node {
    node node;
    uint8_t *name;
    node *pattern;
};

struct slurp_node {
    node node;
    uint8_t *name;
    node *pattern;
    node *until;
};

struct scope_node {
    node node;
    uint8_t *name;
    node *start;
    node *term;
    node *pattern;
};

struct alt_node {
    node node;
    uint8_t *name;
    node **alts;
    size_t count;
};

#endif
