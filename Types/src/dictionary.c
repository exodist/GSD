#include "dictionary.h"
#include "string.h"
#include "type.h"
#include "include/collector_api.h"
#include <assert.h>
#include <string.h>

dict_methods DOBJ_METH = {
    .cmp = obj_cmp,
    .loc = obj_loc,
    .change = obj_change,
};

void obj_change( dict *d, void *meta, void *key, void *old_val, void *new_val ){
    object *ko = key;
    if (ko->type != GC_INT) return;

    // Key is not being added or removed
    if   (old_val && new_val)  return;
    if (!(old_val || new_val)) return; // dereference of an empty key

    gc_dict_meta *m = meta;
    object_simple *kos = key;
    int64_t kval = kos->simple_data.integer;

    // Modify the 'bounds' structure, on successful swap 'dispose' old one
    gc_dict_bounds *new = NULL;
    while(1) {
        gc_dict_bounds *curr = m->bounds;
        int64_t upper = curr->upper->simple_data.integer;
        int64_t lower = curr->lower->simple_data.integer;

        // Remove a key
        if (!new_val) {
            // key is not a bound, no change needed
            if (!(lower == kval || upper == kval)) return;

            new = malloc(sizeof( gc_dict_bounds ));
            // ... ouch XXX out of memory, hard to handle gracefully here.
            // Loop until we have memory, hopefully something else will
            // free up some memory.
            if (!new) continue;
            memset( new, 0, sizeof( gc_dict_bounds ));

            if (upper == kval) {
                new->upper = new_int( kval - 1 );
                // XXX Ouch!
                if (!new->upper) continue;

                new->lower = curr->lower;
            }
            else if (lower == kval) {
                new->lower = new_int( kval + 1 );
                // XXX Ouch!
                if (!new->lower) continue;

                new->upper = curr->upper;
            }
            // else: We are both bounds, both become NULL
        }
        // Add a key
        else {
            // In theory < and > are good enough
            // If we are within the bounds no need to update them
            if (kval >= lower || kval <= upper) return;

            new = malloc(sizeof( gc_dict_bounds ));
            // ... ouch XXX out of memory, hard to handle gracefully here.
            // Loop until we have memory, hopefully something else will
            // free up some memory.
            if (!new) continue;
            memset( new, 0, sizeof( gc_dict_bounds ));

            if (kval > upper) {
                new->upper = new_int( kval + 1 );
                // XXX Ouch!
                if (!new->upper) continue;

                new->lower = curr->lower;
            }
            else {
                new->lower = new_int( kval - 1 );
                // XXX Ouch!
                if (!new->lower) continue;

                new->upper = curr->upper;
            }
        }

        // Do atomic swap
        if ( __sync_bool_compare_and_swap( &(m->bounds), curr, new )) {
            gc_dispose( curr );
            break;
        }
    }
}

int obj_cmp( void *meta, void *key1, void *key2, uint8_t *e ) {
    object *ko1 = key1;
    object *ko2 = key2;

    // If strings, do a string compare
    if( is_string( ko1 ) && is_string( ko2 )) {
        return string_compare( ko1, ko2 );
    }

    // If simple, do a compare of simple integer
    if( is_simple( ko1 ) && is_simple( ko2 )) {
        object_simple *s1 = key1;
        object_simple *s2 = key2;
        if ( s1->simple_data.integer > s2->simple_data.integer )
            return 1;

        if ( s1->simple_data.integer < s2->simple_data.integer )
            return -1;

        if (ko1->type > ko2->type)
            return 1;

        if (ko2->type < ko2->type)
            return -1;

        // Same value, same type
        return 0;
    }

    // Compare addresses
    if ( key1 > key2 ) return  1;
    if ( key2 > key1 ) return -1;
    return 0;
}

size_t obj_loc( size_t slot_count, void *meta, void *key, uint8_t *e ) {
    uint64_t hash = hash_object( meta, key, (exception *)e );
    return hash % slot_count;
}

uint64_t fnv_hash_bytes( uint8_t *data, size_t length, uint64_t key ) {
    if ( length < 1 ) return key;

    for ( size_t i = 0; i < length; i++ ) {
        key ^= data[i];
        key *= FNV_PRIME;
    }

    return key;
}

uint64_t hash_object ( gc_dict_meta *meta, object *o, exception *e ) {
    *e = EX_NONE;
    object_simple *os = (object_simple *)o;
    string_iterator *i;
    string_header *sh = NULL;

    hash_function *hf = meta->hash_function;
    uint64_t state   = meta->initial_state;

    switch( o->type ) {
        case GC_UNDEF: return 0;

        case GC_BOOL:
        return os->simple_data.integer ? 1 : 0;

        // Cannot use a decimal number as a hash key
        case GC_DEC:
            *e = EX_DEC_AS_HKEY;
        return 0;

        // Hash the object address
        case GC_CLASS:
        case GC_ROLE:
        case GC_INST:
        return hf((void *)o, sizeof(object *), state);

        // All these use the value of the simple_data
        case GC_INT:
        case GC_POINTER:
        case GC_HANDLE:
        case GC_DICT:
        return hf(
            (void *)&(os->simple_data.integer),
            sizeof(int64_t),
            state
        );

        // All strings except snip might already have a
        // hash. But all get the hash the same way when
        // there is not already a cached one.
        case GC_STRING:
        case GC_STRINGC:
        case GC_ROPE:
            sh = os->simple_data.ptr;
            if (sh->hash_set)
                return sh->hash;
            // Else build it
        case GC_SNIP:
            i = iterate_string( o );
            if (!i) {
                *e = EX_OUT_OF_MEMORY;
                return 0;
            }

            while(!iterator_complete(i)) {
                uint8_t b = iterator_next_byte(i);
                state = hf( &b, 1, state );
            }

            // XXX: Does this need to be atomic?
            if(sh) {
                sh->hash     = state;
                sh->hash_set = 1;
            }
        return state;

        default:
            *e = EX_INVALID_HKEY;
        return 0;
    }
}
