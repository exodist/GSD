#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "memory.h"
#include "vm.h"

static object *call_non_sub( object *to, stack_frame *sf );
static inline object **fetch_arg( thread *t, stack_frame *sf );

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

object **fetch_arg( thread *t, stack_frame *sf ) {
    oparg *a = (oparg *)(++(sf->subop));
    int lookup = a->lookup;
    assert( !lookup );
    int source = a->source;

    switch( source ) {
        case SRC_RET:
            return &(t->retval);
        case SRC_EXCEPTION:
            return &(t->exception);
        case SRC_DATA:
            (sf->data)++;
            return &(sf->data);
        case SRC_INST:
            return &(t->arg_stack->inst);
    }

    fprintf( stderr, "OOPS: %s:%zi\n", __FILE__, __LINE__);
    abort();
    return NULL;
}

void *spawn_worker( void *arg ) {
    object *to = arg;
    thread *t = to->data;
    instance *i = t->instance;

    arg_list *l = NULL;

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
                f->call = *fetch_arg( t, sf );
                t->sub_stack = f;
            break;

            case OP_SET:
                *(fetch_arg( t, sf )) = *(fetch_arg( t, sf ));
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
            break;

            case OP_PUSH:
                fprintf( stderr, "Oops\n" );
                abort();
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
