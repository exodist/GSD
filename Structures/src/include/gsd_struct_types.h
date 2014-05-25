#ifndef GSD_STRUCT_TYPES_H
#define GSD_STRUCT_TYPES_H

#include <stdint.h>
#include <stdlib.h>

// Overarching, a shared state
typedef struct singularity singularity;

// Don't use the singularity
typedef struct alloc  alloc;
typedef struct bitmap bitmap;
typedef struct bloom  bloom;
typedef struct pool   pool;
typedef struct prm    prm;

// Structures that use a singularity to interact and share some state
typedef struct array array;
typedef struct dict  dict;
typedef struct ref   ref;
typedef struct tree  tree;

// It is all on its own
typedef struct object object;

// Structure used as a return for many operations
typedef struct result result;

// function prototypes
typedef uint64_t(hasher)  (const object *o);
typedef int     (item_cmp)(hasher *h, object *a, object *b);
typedef size_t  (item_loc)(hasher *h, object *o, size_t max);

typedef enum {MERGE_SET, MERGE_INSERT, MERGE_UPDATE} merge_method;

#endif
