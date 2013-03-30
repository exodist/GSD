#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistr.h>
#include <uninorm.h>
#include <uniconv.h>

#include "types.h"
#include "memory.h"

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
        // STRING containing number...
        case SET_FROM_STRL_NUM:
            raw = va_arg( args, uint8_t* );
            size = va_arg( args, uint32_t );
            abort();
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
    for ( size_t i = 0; i < args->push_idx; i++ ) {
        object *arg = NULL;
        object *key = create_scalar( th, SET_FROM_INT, i );
        dict_get( args->args, key, (void **)&arg );
        scalar_string *str = obj_str_val( th, arg );

        const char *charset = locale_charset();

        size_t outlen;
        char *output = u8_conv_to_encoding(
            charset,
            iconveh_escape_sequence,
            str->string,
            str->size,
            NULL,
            NULL,
            &outlen
        );

        assert( handle->fp == stdout );

        fwrite(
            output,
            1,
            outlen,
            handle->fp
        );
    }

    sf->complete = 1;

    return create_scalar( th, SET_FROM_INT, 1 );
}
