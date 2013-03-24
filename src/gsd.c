#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistr.h>
#include <uninorm.h>
#include <uniconv.h>
#include "gsd.h"

#include "structure.h"
#include "instance.h"
#include "vm.h"
#include "bytecode.h"
#include "memory.h"

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

object *io_test( instance *i ) {
    object *stdo = NULL;
    dict_get(
        i->symbol_table,
        create_scalar( i->main_thread, SET_FROM_CSTR, "stdout" ),
        (void *)&stdo
    );

    bytecode *bc = new_bytecode();

    bc_push_op( bc, OP_APUSH );

    bc_push_data( bc, create_scalar( i->main_thread, SET_FROM_CSTR, "Hello" ));
    bc_push_data( bc, create_scalar( i->main_thread, SET_FROM_CSTR, " " ));
    bc_push_data( bc, create_scalar( i->main_thread, SET_FROM_CSTR, "World" ));
    bc_push_data( bc, create_scalar( i->main_thread, SET_FROM_CSTR, "\n" ));
    bc_push_op( bc, OP_PUSH );
    bc_push_arg( bc, 0, SRC_DATA );
    bc_push_op( bc, OP_PUSH );
    bc_push_arg( bc, 0, SRC_DATA );
    bc_push_op( bc, OP_PUSH );
    bc_push_arg( bc, 0, SRC_DATA );
    bc_push_op( bc, OP_PUSH );
    bc_push_arg( bc, 0, SRC_DATA );

    bc_push_data( bc, create_scalar( i->main_thread, SET_FROM_CSTR, "stdout" ));
    bc_push_op( bc, OP_CALL );
    bc_push_arg( bc, 1, SRC_DATA );

    presub *ps = bc_to_presub( bc );

    subroutine *s = malloc( sizeof( subroutine ));
    s->symbols = NULL;
    s->closures = NULL;
    s->presub = ps;

    object *so = alloc_object( i->main_thread, i->subroutine_t, s );

    return so;
}

