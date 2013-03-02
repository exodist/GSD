/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef SETTINGS_H
#define SETTINGS_H

#include "include/gsd_dict.h"
#include "structure.h"
#include "error.h"

dict_settings get_settings( dict *d );
dict_methods get_methods( dict *d );
rstat reconfigure( dict *d, dict_settings settings, size_t max_threads );

rstat do_reconfigure( dict *d, size_t slot_count, void *meta, size_t max_threads );

void *reconf_worker( void *in );

rstat reconf_prep_slot( set *set, size_t idx, void *from, void *to );

rstat make_immutable( dict *d, size_t threads );

rstat immutable_callback( set *s, size_t idx, void **args );

#endif
