#include "../include/gsd_dict.h"
#include "../include/gsd_dict_return.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Specification:

Keys and Values should be ref counted
Should have a change hook (replace line with "key: old -> new")
Should use fnv_hash for hash-table
Should use fnv_hash for compare

 == Should have these threads ==
 * Dot dumper, dump every 2 seconds.
 -- These need unique keys --
 * Add many unique keys, then delete them, do it again (assert value each time)
 * Add many unique keys, then deref them, do it again (assert value each time)
 -- None of these use unique keys --
 * insert random keys (in a range) (probably a list, go to random item in list,
   remove it from list and insert. Complete when list is empty. Track
   success/fail
 * Constantly update random keys from range track success/fail
 * Constantly set random keys from range
 * Constantly delete keys
 * Constantly deref keys
 * Constantly make references
 * Constantly iterate all nodes... count them?
 * cmp_* transactional operations (track success/fail)
\*/

typedef struct kv kv;
struct kv {
    size_t   refcount;
    uint64_t fnv_hash;
    uint8_t  data[5];
};

void   kv_change( dict *d, void *meta, void *key, void *old_Val, void *new_val );
void   kv_ref( dict *d, void *meta, void *ref, int delta );
size_t kv_loc( dict_settings *s, void *key );
int    kv_cmp( dict_settings *s, void *key1, void *key2 );
char  *kv_dot( void *key, void *val );
int    kv_handler( void *key, void *value, void *args );

void *thread_dot_dumper( void *ptr );
void *thread_uniq_delete( void *ptr );
void *thread_uniq_deref( void *ptr );
void *thread_rand_insert( void *ptr );
void *thread_rand_update( void *ptr );
void *thread_rand_set( void *ptr );
void *thread_rand_get( void *ptr );
void *thread_rand_delete( void *ptr );
void *thread_rand_deref( void *ptr );
void *thread_rand_ref( void *ptr );
void *thread_iterate_all( void *ptr );
void *thread_transactions( void *ptr );

int main() {
    dict_settings set = { 16, 2, NULL };
    dict_methods  met = { kv_cmp, kv_loc, kv_change, kv_ref };
    dict *d = NULL;
    dict_create( &d, 12, &set, &met );

    dict_free( &d );
}
