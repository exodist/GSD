#include <string.h>
#include <assert.h>

#include "instance.h"
#include "types.h"
#include "memory.h"
#include "parser.h"

static int insert_type( instance *i, const char *name, object *type_o );
static object *init_type( object *t, object *type, object *parent, int refcount );

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

instance *init_instance() {
    instance *i = malloc( sizeof( instance ));
    if ( !i ) return NULL;
    memset( i, 0, sizeof( instance ));

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
    i->parser_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );
    i->keyword_t = init_type( i->main_thread, i->type_t, i->object_t, 0 );
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
    errors += insert_type( i, "Type",   i->type_t );
    errors += insert_type( i, "Graph",  i->graph_t );
    errors += insert_type( i, "Object", i->object_t );
    errors += insert_type( i, "Scalar", i->scalar_t );
    errors += insert_type( i, "IO",     i->io_t );
    errors += insert_type( i, "Thread", i->thread_t );
    errors += insert_type( i, "Parser", i->parser_t );

    errors += insert_type( i, "Keyword", i->keyword_t );
    errors += insert_type( i, "CFunction", i->cfunction_t );
    errors += insert_type( i, "Subroutine", i->subroutine_t );
    errors += insert_type( i, "StackFrame", i->stack_frame_t );

    io *sto_data = malloc( sizeof ( io ));
    assert( sto_data );
    if ( sto_data ) {
        sto_data->fp = stdout;
        object *sto = alloc_object( i->main_thread, i->io_t, sto_data );
        //errors += insert_type( i, "stdout", sto );
        object *name = create_scalar( i->main_thread, SET_FROM_CSTR, "stdout" );
        dict_insert( i->symbol_table, name, sto );
        assert(((io *)sto->data)->fp == stdout );
    }

    keyword *dquote = malloc( sizeof( keyword ));
    assert( dquote );
    if ( dquote ) {
        dquote->ckeyword = dquote_keyword;
        object *dqo = alloc_object( i->main_thread, i->keyword_t, dquote );
        object *name = create_scalar( i->main_thread, SET_FROM_CSTR, "\"" );
        dict_stat s = dict_insert( i->symbol_table, name, dqo );
        assert( !s.bit.error );
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

static int insert_type( instance *i, const char *name, object *type_o ) {
    object *name_obj = create_scalar( i->main_thread, SET_FROM_CSTR, name );
    type *t = type_o->data;
    t->name = name_obj;
    dict_stat s = dict_insert( i->symbol_table, name_obj, type_o );
    if ( s.bit.fail ) return 1;
    return 0;
}
