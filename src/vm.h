#ifndef GSD_VM_H
#define GSD_VM_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

#include "structure.h"
#include "types.h"
#include "bytecode.h"

object *spawn( object *parent, object *run );

void *spawn_worker( void * );

#endif
