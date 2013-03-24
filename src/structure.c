#include <assert.h>
#include <unistr.h>
#include <uninorm.h>
#include <uniconv.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "structure.h"

uint64_t HASH_SEED = 14695981039346656037UL;

dict_methods DMETH = {
    .cmp = obj_compare,
    .loc = obj_locate,
    .ref = obj_ref
};

dict_methods OMETH = {
    .cmp = obj_ocompare,
    .loc = obj_locate,
    .ref = obj_ref
};

int obj_compare( void *meta, void *obj1, void *obj2 ) {
    if ( obj1 == obj2 ) return 0;
    object *a = obj1;
    object *b = obj2;

    // Compare hash, conflict does not mean same
    if ( a->hash > b->hash ) return  1;
    if ( a->hash < b->hash ) return -1;

    // Scalar type with same hash
    dict_meta *m = meta;
    if ( a->type == b->type && a->type == m->instance->scalar_t ) {
        scalar *as = a->data;
        scalar *bs = b->data;
        if ( as->init_as == bs->init_as ) {
            int64_t idiff;
            double ddiff;
            int sdiff;
            switch( as->init_as ) {
                case SET_AS_INT:
                    idiff = as->integer - bs->integer;
                    if ( !idiff )    return  0;
                    if ( idiff > 0 ) return  1;
                                     return -1;
                case SET_AS_DEC:
                    ddiff = as->decimal - bs->decimal;
                    if ( !ddiff )    return  0;
                    if ( ddiff > 0 ) return  1;
                                     return -1;
                case SET_AS_STR:
                    // Scalar-Strings will all be stored in UNINORM_NFC, so
                    // this type of compare should be fine.
                    sdiff = u8_cmp2(
                        as->string->string, as->string->size,
                        bs->string->string, bs->string->size
                    );
                    if ( !sdiff )    return  0;
                    if ( sdiff > 0 ) return  1;
                                     return -1;
            }
        }
    }

    // If hash conflict compare memory addresses
    if ( obj1 > obj2 ) return  1;
    if ( obj1 < obj2 ) return -1;
    return 0;
}

int obj_ocompare( void *meta, void *obj1, void *obj2 ) {
    if ( obj1 == obj2 ) return 0;
    object *a = obj1;
    object *b = obj2;

    dict_meta *m = meta;
    if ( a->type == b->type && a->type == m->instance->scalar_t ) {
        scalar *as = a->data;
        scalar *bs = b->data;
        if ( as->init_as == bs->init_as ) {
            int64_t idiff;
            double ddiff;
            int sdiff;
            switch( as->init_as ) {
                case SET_AS_INT:
                    idiff = as->integer - bs->integer;
                    if ( !idiff )    return  0;
                    if ( idiff > 0 ) return  1;
                                     return -1;
                case SET_AS_DEC:
                    ddiff = as->decimal - bs->decimal;
                    if ( !ddiff )    return  0;
                    if ( ddiff > 0 ) return  1;
                                     return -1;
                case SET_AS_STR:
                    // Scalar-Strings will all be stored in UNINORM_NFC, so
                    // this type of compare should be fine.
                    sdiff = u8_cmp2(
                        as->string->string, as->string->size,
                        bs->string->string, bs->string->size
                    );
                    if ( !sdiff )    return  0;
                    if ( sdiff > 0 ) return  1;
                                     return -1;
            }
        }
        else {
            if ( as->init_as == SET_AS_INT ) {
                abort();
            }
            else {
                abort();
            }
        }
    }

    if ( obj1 > obj2 ) return  1;
    if ( obj1 < obj2 ) return -1;
    return 0;
}

size_t obj_locate( size_t slot_count, void *meta, void *obj ) {
    return ((object *)obj)->hash % slot_count;
}

void obj_ref( dict *d, void *obj, int delta ) {
    object *o = obj;
    object *t = o->type;
    type *type = t->data;
    if ( !type->refcounted ) return;

    // Atomic update of refcount
    size_t count = __sync_add_and_fetch( &(o->refcount), delta );
    if ( count == 0 ) {
        // TODO: Garbage collection
    }
}

char *dot_decode( void *key, void *val ) {
    object *k   = key;
    object *kto = k->type;
    type   *kt  = kto->data;
    object *kn  = kt->name;
    scalar *ks  = kn->data;
    scalar_string *kst = ks->string;

    char *out = malloc( 200 );
    memset( out, 0, 200 );
    memcpy( out, kst->string, kst->size );

    size_t idx = kst->size;
    sprintf( out + idx, "(%p) -> [%p]", key, val );

    return out;
}

uint64_t hash_bytes( uint8_t *data, size_t length, uint64_t seed ) {
    if ( length < 1 ) return seed;

    uint64_t key = seed;
    uint64_t i;
    for ( i = 0; i < length; i++ ) {
        key ^= data[i];
        key *= 1099511628211;
    }

    return key;
}

scalar_string *build_string( uint8_t *raw, size_t bytes ) {
    size_t size = 0;
    uint8_t *norm = u8_normalize(
        UNINORM_NFC,
        raw,
        bytes,
        NULL,
        &size
    );

    if ( !norm ) return NULL;

    scalar_string *out = malloc( sizeof( scalar_string ));
    if ( out == NULL ) {
        free( norm );
        return NULL;
    }

    out->string = norm;
    out->size = size;
    out->refs = 1;
    return out;
}

scalar_string *obj_str_val( object *to, object *o ) {
    thread *t = to->data;
    if ( o->type == t->instance->scalar_t ) {
        scalar *s = o->data;
        if ( s->init_as == SET_AS_STR ) {
            return s->string;
        }
    }
    abort();
    return NULL;
//
//    if ( o->type != t->instance->scalar_t ) {
//        scalar *s = o->data;
//        if ( !o->string ) {
//
//        }
//
//        __sync_add_and_fetch( &(o->string->refs), 1 );
//        return o->string;
//    }
//
//    scalar_string *st = malloc( sizeof( scalar_string ));
}

int64_t obj_int_val( object *t, object *o );
double obj_dec_val( object *t, object *o );

