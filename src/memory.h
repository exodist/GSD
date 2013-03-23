#ifndef GSD_MEMORY_H
#define GSD_MEMORY_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

#include "structure.h"

object *alloc_object( object *t, object *type_o, void *data );

#endif
