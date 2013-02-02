#ifndef GSD_DICT_API_H
#define GSD_DICT_API_H

#include <stdint.h>
#include <stdlib.h>

/* Return codes:
 * CODE < 0 - Fatal, cannot trust the dictionary after one of these
 * CODE = 0 - Success, nothing wrong
 * CODE > 0 - Action failed, but otherwise dictionary is fine
\*/

//        Error Name   Code  Fatal? Description
// These are explosions
#define DICT_INT_ERROR  -2 /* Y   Internal Error                              */
#define DICT_API_ERROR  -1 /* Y   API was used incorrectly                    */
// These are varying levels of success
#define DICT_NO_ERROR    0 /* N   Success, no error                           */
#define DICT_PATHO_ERROR 1 /* N   Operation would result in pathological tree */
// These are graceful failures 
#define DICT_MEM_ERROR   2 /* N   Out of memory,                              */
#define DICT_TRANS_FAIL  3 /* N   Transaction failed                          */
#define DICT_UNIMP_ERROR 4 /* N   Operation not implemented                   */

/* PATHO error is returned either when rebalancing a tree doesn't actually
 * balance it, or if one tree is significantly taller than its neighbors. Both
 * cases imply pathological data is being fed to your dictionary. A PATHO error
 * does NOT mean your operation failed.
 */

/* If the action triggered a rebalance this will be added to the return code,
 * if the return code is >= 100 it means that your action succeded, but
 * triggered a rebalance, the return code for the rebalance is
 *    (code - DICT_RBAL)
 *
 * If the rebalance fails your dictionary is uneffected and will still work
 * properly, it just won't be balanced until another insert triggers the
 * rebalance over again.
 *
 * NOTE1: It is still a good idea to abort on a fatal error (1000 > c > 100)
 * NOTE2: Do not ignore DICT_PATHO_ERROR, doing so will have drastic effects on
 *        performance. It occurs when rebalancing still results in an
 *        unbalanced tree, which means rebalances will happen frequently.
 *        Someone is probably exploiting a flaw in your program.
 */
#define DICT_RBAL 1000

typedef struct dict_settings dict_settings;
typedef struct dict_methods  dict_methods;
typedef struct dict dict;

typedef void   (dict_hook)( dict *d, dict_settings *s, void *item );
typedef int    (dict_handler)( void *key, void *value, void *args );
typedef int    (dict_cmp)( dict_settings *s, void *key1, void *key2 );
typedef size_t (dict_loc)( dict_settings *s, void *key );
typedef char * (dict_dot)( void *key, void *val );

struct dict;

/* Callbacks:
 * The first 2 are required, they let you control where items are inserted into
 * the hash table and binary trees.
 *
 * The others are useful for tracking metrics about the dictionary, or for
 * reference-based garbage collection of your keys and values.
\*/
struct dict_methods {
    // These 2 are required
    dict_cmp *cmp;  // Used to compare keys ( -1, 0, 1 : left, same, right )
    dict_loc *loc;  // Used to find slot of key (hashing function)

    // Hooks for when keys are added/removed
    // These should be used for metadata
    // ** NOT FOR REFERENCE COUNTING **
    // This is also called when a key is set to NULL as opposed to being
    // removed completely.
    dict_hook *ins; // Callback when a key obtains a value (insert, or previously null)
    dict_hook *rem; // Callback when a key looses a value (delete or deref)

    // Hooks for reference counting
    // These are called whenever a dictionary adds or removes a reference to
    // key or value. These can be used for reference counting purposes.
    dict_hook *ref_add; // Callback when the dictionary adds a ref
    dict_hook *ref_del; // Callback when the dictionart deletes a ref
};

/* Dictionary Settings:
 * If anything is left empty, a sane default is used, unless this is used to
 * change the settings of an existing dictionary in which case empty settings
 * means no change.
 *
 * slot_count determines how big of a hash table to use. More hash slots means
 * more memory usage, but potentially faster updates.
 *
 * epoch_count determines how many slots the garbage collector is assigned. In
 * a single-threaded environment this can be set to 1. In a multithreaded
 * environment this is very important. Too few and you can have threads
 * spinning waiting for garbage collection. Too many and you have wasted space
 * (See struct epoch in structures.h for the size of each epoch).
 *
 * It is unlikely (but possible) to use more epochs than you have threads
 * modifying the dictionary. Adding epochs is a cheap operation, removing
 * epochs will block while epochs that need to be removed are in use.
 *
 * meta is yours to use, it is not used internally, but is passed to several
 * callbacks.
\*/
struct dict_settings {
    size_t slot_count;    // How many hash slots to allocate
    size_t epoch_count;   // How many epochs the GC can use
    size_t max_imbalance; // How much imbalance is allowed

    // Metadata you can attach to the dictionary
    void *meta;
};

// -- Creation and meta data --

// This macro can be used to create a dict that gives you more verbose errors
// if you use it incorrectly.
#define dict_create_verbose( a, b, c, d, e ) dict_create_vb( a, b, c, d, e, __FILE__, __LINE__ )
int dict_create_vb( dict **d, dict_settings *s, dict_methods *m, char *f, size_t l );

// This is the normal way to create a dict that does not write any output
int dict_create( dict **d, dict_settings *settings, dict_methods *methods );

// Copying and cloning
int dict_merge( dict *from, dict *to );
int dict_merge_refs( dict *from, dict *to );

// Used to free a dict structure.
// Does not free your keys, values, meta, or methods.
int dict_free( dict **d );

// Used to access your settings
dict_settings *dict_get_settings( dict *d );

// Used to get your dict_methods item.
dict_methods *dict_get_methods( dict *d );

// -- Informative --

int dict_dump_dot( dict *d, char **buffer, dict_dot *show );

// -- Operation --

// This allows you to rebuild your dictionary using new metadata and/or slot
// count. This is useful if you get a DICT_PATHO_ERROR which means the data in
// your dictionary appears to be pathalogical
int dict_reconfigure( dict *d, dict_settings *settings );

// Get never blocks
// Set will insert or update as necessary
// Insert may block in a rebuild or rebalance, fails if key is present
// (DICT_EXIST_ERROR)
// Update never blocks, but fails if the key is not present (DICT_EXIST_ERROR)
// delete removes the value for a key, key will not show in iterations
int dict_get( dict *d, void *key, void **val );
int dict_set( dict *d, void *key, void *val );
int dict_insert( dict *d, void *key, void *val );
int dict_update( dict *d, void *key, void *val );
int dict_delete( dict *d, void *key );

// cmp_update is just like update() except that you can tell it to only set the
// new value if the old value is what you specify, useful in a threaded program
// to implement a primitive form of transaction for a specific key.
// cmp_delete is just like delete, but it only removes the value if it is what
// you expect.
int dict_cmp_update( dict *d, void *key, void *old_val, void *new_val );
int dict_cmp_delete( dict *d, void *key, void *old_val );
int dict_cmp_dereference( dict *fromd, void *fromk, dict *cmpd, void *cmpk );

// reference allows you to "tie" a key in one dictionary to a key in another
// dictionary.
// dereference allows you to "untie" the key in a dictionary. It can also be
// used on any key to fully remove it from the dictionary, which is something
// delete does not do.
int dict_reference( dict *orig, void *okey, dict *dest, void *dkey );
int dict_dereference( dict *d, void *key );

// handler can return false to break loop
int dict_iterate( dict *d, dict_handler *h, void *args );

#endif
