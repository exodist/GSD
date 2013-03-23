#ifndef GSD_INSTANCE_H
#define GSD_INSTANCE_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>
#include "GSD_Dict/src/include/gsd_dict.h"
#include "GSD_Dict/src/include/gsd_dict_return.h"

#include "structure.h"

instance *init_instance();

void cleanup( instance *i );

#endif
