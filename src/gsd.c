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
//    object *stdo = NULL;
//    dict_get(
//        i->symbol_table,
//        create_scalar( i->main_thread, SET_FROM_CSTR, "stdout" ),
//        (void *)&stdo
//    );
//
//    subop op0 = { NULL, OP_APUSH };
//    subop op1 = { create_scalar( i->main_thread, SET_FROM_CSTR, "Hello World\n" ), OP_OPUSH };
//    subop op2 = { stdo, OP_OCALL };
//    subop op3 = { create_scalar( i->main_thread, SET_FROM_INT, 1 ), OP_OPUSH };
//    subop op4 = { NULL, OP_END   };
//
//    subop *ops = malloc( sizeof( subop ) * 5 );
//    ops[0] = op0;
//    ops[1] = op1;
//    ops[2] = op2;
//    ops[3] = op3;
//    ops[4] = op4;
//
//    subroutine *s = malloc( sizeof( subroutine ));
//    s->symbols = NULL;
//    s->closures = NULL;
//    s->entry = ops;
//
//    object *so = alloc_object( i->main_thread, i->subroutine_t, s );
//
//    return so;
    return NULL;
}

