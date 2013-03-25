#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "memory.h"
#include "vm.h"

static object *call_non_sub( object *to, stack_frame *sf );
static inline object *fetch_arg( thread *t, stack_frame *sf );

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

object *fetch_arg( thread *t, stack_frame *sf ) {
    oparg *a = (oparg *)(++(sf->subop));
    int lookup = a->lookup;
    int source = a->source;

    object *src = NULL;
    switch( source ) {
        case SRC_RET:
            src = t->retval;
        break;
        case SRC_EXCEPTION:
            src = t->exception;
        break;
        case SRC_DATA:
            src = sf->data[0];
            (sf->data)++;
        break;
        case SRC_INST:
            src = t->arg_stack->inst;
        break;
        case SRC_COND:
            src = sf->condition;
        break;
        default:
            fprintf( stderr, "OOPS: %s:%u\n", __FILE__, __LINE__);
            abort();
        break;
    }

    if ( !lookup ) return src;

    assert( src );
    instance *i = t->instance;
    object *out = NULL;
    dict_get( i->symbol_table, src, (void **)&out );
    assert( out );
    return out;
}

void *spawn_worker( void *arg ) {
    object *to = arg;
    thread *t = to->data;
    instance *i = t->instance;

    arg_list *l = NULL;
    int64_t idx = 0;
    object *key = NULL;
    object *val = NULL;

    while ( t->sub_stack ) {
        stack_frame *sf = t->sub_stack;

        if ( sf->call->type != i->subroutine_t ) {
            if ( sf->complete ) {
                // TODO free stack frame
                return NULL;
            }
            object *exception = call_non_sub( to, sf );
            assert( !exception );
            continue;
        }

        if ( !sf->subop ) {
            subroutine *s = t->sub_stack->call->data;
            sf->subop = s->presub->entry;
            sf->data  = s->presub->data;
        }
        else {
            (sf->subop)++;
        }

        switch( *(sf->subop) ) {
            case OP_END:
                t->sub_stack = t->sub_stack->parent;
                // TODO free stack frame
            continue;

            case OP_CALL:
                l = t->arg_stack;

                stack_frame *f = malloc( sizeof( stack_frame ));
                if ( !f ) {
                    fprintf( stderr, "Memory\n" );
                    abort();
                }

                f->args = l;
                f->parent = t->sub_stack;
                f->call = fetch_arg( t, sf );

                t->arg_stack = l->parent;
                t->sub_stack = f;
            break;

            case OP_SET:
                //*(fetch_arg( t, sf )) = *(fetch_arg( t, sf ));
            break;

            case OP_APUSH:
                l = malloc( sizeof( arg_list ));
                if ( !l ) {
                    fprintf( stderr, "Memory\n" );
                    abort();
                }

                memset( l, 0, sizeof( arg_list ));
                l->parent = t->arg_stack;
                t->arg_stack = l;

                dict_meta *m = malloc( sizeof( dict_meta ));
                if ( !m ) {
                    free( i->main_thread );
                    free( i );
                }
                m->seed     = HASH_SEED;
                m->instance = i;
                m->use_fnv  = 1;
            
                l->args = dict_build( 64, DMETH, m );
                assert( l->args );
            break;

            case OP_PUSH:
                val = fetch_arg( t, sf );
                while ( 1 ) {
                    idx = (t->arg_stack->push_idx)++;
                    key = create_scalar( to, SET_FROM_INT, idx );
                    assert( key );
                    dict_stat s = dict_insert( t->arg_stack->args, key, val );
                    if ( !s.bit.fail ) break;
                }
            break;

            case OP_PUSHREF:
                fprintf( stderr, "Oops\n" );
                abort();
            break;

            case OP_REF:
                fprintf( stderr, "Oops\n" );
                abort();
            break;

            case OP_MAP:
                fprintf( stderr, "Oops\n" );
                abort();
            break;

            case OP_GRAPH:
                fprintf( stderr, "Oops\n" );
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

object *call_non_sub( object *to, stack_frame *sf ) {
    thread *t = to->data;
    instance *i = t->instance;

    object *callo = sf->call;
    if ( callo->type == i->cfunction_t ) {
        object *out = NULL;
        cfunction *cf = callo->data;
        object *ret = cf( to, sf, &out );
        // handle return value
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
