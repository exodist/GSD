#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistr.h>
#include <uninorm.h>
#include <uniconv.h>
#include "gsd.h"

instance *I = NULL;

uint64_t HASH_SEED = 14695981039346656037UL;

dict_methods DMETH = {
    .cmp = obj_compare,
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
                    ddiff = as->integer - bs->integer;
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

object *io_test( instance *i );

int main() {
    instance *i = init_instance();
    if ( !i ) {
        fprintf( stderr, "Error\n" );
        return 1;
    }

    //char *dot = dict_dump_dot( i->symbol_table, dot_decode );
    //printf( "%s\n", dot );

    object *run = io_test(i);
    thread *mt = i->main_thread->data;
    mt->sub_stack = malloc( sizeof( stack_frame ));
    memset( mt->sub_stack, 0, sizeof( stack_frame ));
    mt->sub_stack->call = run;
    spawn_worker( i->main_thread );

    return 0;
}

instance *init_instance() {
    instance *i = malloc( sizeof( instance ));
    if ( !i ) return NULL;
    memset( i, 0, sizeof( instance ));

    I = i;

    thread *main_thread = malloc( sizeof( thread ));
    if ( !main_thread ) {
        free( i );
        return NULL;
    }

    memset( main_thread, 0, sizeof( thread ));
    main_thread->pthread = pthread_self();
    main_thread->instance = i;

    i->main_thread = malloc( sizeof( object ));
    if ( !i->main_thread ) {
        free( i );
        free( main_thread );
        return NULL;
    }

    memset( i->main_thread, 0, sizeof( object ));
    i->main_thread->data = main_thread;

    dict_meta *m = malloc( sizeof( dict_meta ));
    if ( !m ) {
        free( i->main_thread );
        free( i );
    }
    m->seed     = HASH_SEED;
    m->instance = i;
    m->use_fnv  = 1;

    i->symbol_table = dict_build( 10240, DMETH, m );
    if ( !i->symbol_table ) {
        free( i->main_thread );
        free( i );
        free( m );
        return NULL;
    }

    i->type_t   = init_type( i->main_thread, NULL,      NULL, 0 );
    i->object_t = init_type( i->main_thread, i->type_t, NULL, 0 );
    type *t = i->type_t->data;
    t->parent = i->object_t;
    i->type_t->type = i->type_t;

    i->io_t = init_type( i->main_thread, i->type_t, i->object_t, 1 );
    i->graph_t  = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->scalar_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->thread_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->cfunction_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->subroutine_t  = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->stack_frame_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );

    i->main_thread->type = i->thread_t;
    i->main_thread->hash = hash_bytes(
        (uint8_t *)(i->main_thread),
        sizeof( object ),
        HASH_SEED
    );

    int errors = 0;
    errors += insert_symbol( i, "Type",   i->type_t );
    errors += insert_symbol( i, "Graph",  i->graph_t );
    errors += insert_symbol( i, "Object", i->object_t );
    errors += insert_symbol( i, "Scalar", i->scalar_t );
    errors += insert_symbol( i, "IO",     i->io_t );
    errors += insert_symbol( i, "Thread", i->thread_t );

    errors += insert_symbol( i, "CFunction", i->cfunction_t );
    errors += insert_symbol( i, "Subroutine", i->subroutine_t );
    errors += insert_symbol( i, "StackFrame", i->stack_frame_t );

    io *sto_data = malloc( sizeof ( io ));
    if ( sto_data ) {
        sto_data->fp = stdout;
        object *sto = alloc_object( i->main_thread, i->io_t, sto_data );
        errors += insert_symbol( i, "stdout", sto );
    }

    if ( errors ) {
        cleanup( i );
        return NULL;
    }

    errors += init_io( i );

    if ( errors ) {
        cleanup( i );
        return NULL;
    }

    return i;
}

void cleanup( instance *i ) {
    return;
}

int insert_symbol( instance *i, const char *name, object *type_o ) {
    object *name_obj = create_scalar( i->main_thread, SET_FROM_CSTR, name );
    type *t = type_o->data;
    t->name = name_obj;
    dict_stat s = dict_insert( i->symbol_table, name_obj, type_o );
    if ( s.bit.fail ) return 1;
    return 0;
}

object *init_type( object *t, object *typ, object *parent, int refcount ) {
    type *tdata = malloc( sizeof( type ));
    if ( !tdata ) return NULL;

    tdata->parent  = parent;
    tdata->symbols = NULL;
    tdata->roles   = NULL;
    tdata->refcounted = refcount;

    object *o = alloc_object( t, typ, tdata );
    if (!o) {
        free( tdata );
        return NULL;
    }

    return o;
}

object *spawn( object *parent, object *run ) {
    thread *t = malloc( sizeof( thread ));
    if ( !t ) return NULL;
    memset( t, 0, sizeof( thread ));

    thread *pt = parent->data;
    t->instance = pt->instance;

    t->sub_stack = malloc( sizeof( stack_frame ));
    if ( !t->sub_stack ) {
        free( t );
        return NULL;
    }

    memset( t->sub_stack, 0, sizeof( stack_frame ));
    t->sub_stack->call = run;

    object *to = alloc_object( parent, t->instance->thread_t, t );

    pthread_create(
        &(t->pthread),
        NULL,
        spawn_worker,
        to
    );

    return to;
}

object *alloc_object( object *th, object *type_o, void *data ) {
    thread *t = th->data;
    object *o = malloc( sizeof( object ));
    if ( !o ) return NULL;
    memset( o, 0, sizeof( object ));
    o->type = type_o;
    o->data = data;

    // If scalar calculate content hash
    if ( type_o && type_o == t->instance->scalar_t ) {
        scalar *s = o->data;
        void *ptr    = NULL;
        size_t   size = 0;
        switch( s->init_as ) {
            case SET_AS_INT:
                ptr  = &(s->integer);
                size = sizeof( int64_t );
            break;
            case SET_AS_DEC:
                ptr  = &(s->decimal);
                size = sizeof( double );
            break;
            case SET_AS_STR:
                ptr  = s->string->string;
                size = s->string->size;
            break;
        }
        o->hash = hash_bytes( ptr, size, HASH_SEED );
    }
    else {
        o->hash = hash_bytes(
            (uint8_t *)&o,
            sizeof( object ),
            HASH_SEED
        );
    }

    if ( type_o ) {
        type *tp = type_o->data;
        if ( !tp->refcounted ) {
            int success = 0;
            while ( !success ) {
                o->gc_next = t->alloc;
                success = __sync_bool_compare_and_swap( &(t->alloc), o->gc_next, o );
            }
        }
    }

    return o;
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

object *create_scalar( object *t, scalar_init vt, ... ) {
    va_list args;
    va_start( args, vt );

    scalar *s_data = malloc( sizeof( scalar ));
    if ( !s_data ) return NULL;
    memset( s_data, 0, sizeof( scalar ));

    object *obj  = NULL;
    scalar *from = NULL;
    uint8_t *raw = NULL;
    size_t  size = 0;

    switch ( vt ) {
        case SET_FROM_OBJ:
            obj  = va_arg( args, object* );
            from = obj->data;
            *s_data = *from;

            if ( s_data->string )
                __sync_add_and_fetch( &(s_data->string->refs), 1 );
        break;
        case SET_FROM_INT:
            s_data->integer = va_arg( args, int64_t );
            s_data->int_set = 1;
            s_data->init_as = SET_AS_INT;
        break;
        case SET_FROM_DEC:
            s_data->decimal = va_arg( args, double );
            s_data->dec_set = 1;
            s_data->init_as = SET_AS_DEC;
        break;
        case SET_FROM_STRL:
            s_data->init_as = SET_AS_STR;
            raw = va_arg( args, uint8_t* );
            size = va_arg( args, uint32_t );
            s_data->string = build_string( raw, size );
            if ( !s_data->string ) {
                free( s_data );
                return NULL;
            }
        break;
        case SET_FROM_CSTR:
            s_data->init_as = SET_AS_STR;
            raw = va_arg( args, uint8_t* );
            size = strlen( (char *)raw );
            s_data->string = build_string( raw, size );
            if ( !s_data->string ) {
                free( s_data );
                return NULL;
            }
        break;
    }

    va_end( args );

    thread *th = t->data;
    object *out = alloc_object( t, th->instance->scalar_t, s_data );
    if( out ) return out;

    free( s_data );
    return NULL;
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

void *spawn_worker( void *arg ) {
    object *to = arg;
    thread *t = to->data;
    instance *i = t->instance;

    arg_list *l = NULL;

    while ( t->sub_stack ) {
        stack_frame *sf = t->sub_stack;

        if ( sf->call->type != i->subroutine_t ) {
            if ( sf->current.complete ) {
                // TODO free stack frame
                return NULL;
            }
            object *exception = call_non_sub( to, sf );
            assert( !exception );
            continue;
        }

        if ( !sf->current.subop ) {
            subroutine *s = t->sub_stack->call->data;
            sf->current.subop = s->entry;
        }
        else {
            (sf->current.subop)++;
        }

        switch( sf->current.subop->op ) {
            case OP_END:
                t->sub_stack = t->sub_stack->parent;
                // TODO free stack frame
            continue;

            case OP_APUSH:
                l = malloc( sizeof( arg_list ));
                if ( !l ) {
                    fprintf( stderr, "Memory\n" );
                    abort();
                }

                memset( l, 0, sizeof( arg_list ));
                l->parent = t->arg_stack;
                t->arg_stack = l;
            break;

            case OP_OINST:
                l = t->arg_stack;
                l->inst = sf->current.subop->item;
            break;

            case OP_OPUSH:
                l = t->arg_stack;
                if ( l->size >= l->length ) {
                    void *check = realloc( l->list, (l->size + 16) * sizeof( object * ));
                    if ( !check ) {
                        fprintf( stderr, "Memory\n" );
                        abort();
                    }
                    l->list = check;
                }
                l->list[l->length++] = sf->current.subop->item;
            break;

            case OP_OCALL:
                // Pop the arg_list
                l = t->arg_stack;
                t->arg_stack = l->parent;

                stack_frame *f = malloc( sizeof( stack_frame ));
                if ( !f ) {
                    fprintf( stderr, "Memory\n" );
                    abort();
                }

                f->args = l;
                f->parent = t->sub_stack;
                f->call = sf->current.subop->item;
                t->sub_stack = f;
            continue;

            case OP_SINST:
                fprintf( stderr, "TODO: SINST\n" );
                abort();
            break;

            case OP_SPUSH:
                fprintf( stderr, "todo: SPUSH\n" );
                abort();
            break;

            case OP_SCALL:
                fprintf( stderr, "todo: SCALL\n" );
                abort();
            break;

            case OP_OPOP:
                fprintf( stderr, "todo: OPOP\n" );
                abort();
            break;

            default:
                fprintf( stderr, "WTF\n" );
                abort();
            break;
        }
    }

    return NULL;
}

object *io_test( instance *i ) {
    object *stdo = NULL;
    dict_get(
        i->symbol_table,
        create_scalar( i->main_thread, SET_FROM_CSTR, "stdout" ),
        (void *)&stdo
    );

    subop op0 = { NULL, OP_APUSH };
    subop op1 = { create_scalar( i->main_thread, SET_FROM_CSTR, "Hello World\n" ), OP_OPUSH };
    subop op2 = { stdo, OP_OCALL };
    subop op3 = { create_scalar( i->main_thread, SET_FROM_INT, 1 ), OP_OPUSH };
    subop op4 = { NULL, OP_END   };

    subop *ops = malloc( sizeof( subop ) * 5 );
    ops[0] = op0;
    ops[1] = op1;
    ops[2] = op2;
    ops[3] = op3;
    ops[4] = op4;

    subroutine *s = malloc( sizeof( subroutine ));
    s->symbols = NULL;
    s->closures = NULL;
    s->entry = ops;

    object *so = alloc_object( i->main_thread, i->subroutine_t, s );

    return so;
}

int init_io( instance *i ) {
    object *io = i->io_t;
    type *t = io->data;

    dict_meta *m = malloc( sizeof( dict_meta ));
    if ( !m ) {
        free( i->main_thread );
        free( i );
    }
    m->seed     = HASH_SEED;
    m->instance = i;
    m->use_fnv  = 1;

    t->symbols = dict_build( 1024, DMETH, m );
    assert( t->symbols );

    t->call = alloc_object( i->main_thread, i->cfunction_t, io_call );
    assert( t->call );

    return 0;
}

object *io_call( object *th, stack_frame *sf, object **exception ) {
    thread *t = th->data;
    instance *i = t->instance;

    io *handle = sf->args->inst->data;

    arg_list *args = sf->args;
    for ( size_t i = 0; i < args->length; i++ ) {
        object *arg = args->list[i];
        uint8_t *str = obj_str_val( th, arg );
        size_t   len = obj_str_len( th, arg );

        const char *charset = locale_charset();

        size_t outlen;
        char *output = u8_conv_to_encoding(
            charset,
            iconveh_escape_sequence,
            str,
            len,
            NULL,
            NULL,
            &outlen
        );

        fwrite(
            output,
            1,
            outlen,
            handle->fp
        );
    }

    sf->current.complete = 1;

    return create_scalar( th, SET_FROM_INT, 1 );
}

object *call_non_sub( object *to, stack_frame *sf ) {
    thread *t = to->data;
    instance *i = t->instance;

    object *callo = sf->call;
    if ( callo->type == i->cfunction_t ) {
        object *out = NULL;
        cfunction *cf = callo->data;
        object *ret = cf( to, sf, &out );
        // Push ret to parent arg_list
        return out;
    }

    type *ct = callo->type->data;
    if ( !ct->call ) {
        fprintf( stderr, "No call" );
        abort();
    }

    sf->args->inst = sf->call;
    sf->call = ct->call;

    return NULL;
}

scalar_string *obj_str_val( object *t, object *o ) {
    thread *t = to->data;

    if ( o->type != t->instance->scalar_t ) {
        scalar *s = o->data;
        if ( !o->string ) {

        }

        __sync_add_and_fetch( &(o->string->refs), 1 );
        return o->string;
    }

    scalar_string *st = malloc( sizeof( scalar_string ));
}

int64_t obj_int_val( object *t, object *o );
double obj_dec_val( object *t, object *o );

