/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef EPOCH_H
#define EPOCH_H

#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"
#include "structures.h"

epoch *dict_create_epoch();
void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type );
epoch *dict_join_epoch( dict *d );
void dict_leave_epoch( dict *d, epoch *e );

#endif
