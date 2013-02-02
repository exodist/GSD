/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the gsd_dict_api.h header file in external programs.
\*/

#ifndef GSD_DICT_EPOCH_H
#define GSD_DICT_EPOCH_H

#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"

epoch **dict_create_epochs( uint8_t epoch_count );
void dict_dispose( dict *d, epoch *e, void *meta, void *garbage, int type );
void dict_join_epoch( dict *d, uint8_t *idx, epoch **ep );
void dict_leave_epoch( dict *d, epoch *e );

#endif
