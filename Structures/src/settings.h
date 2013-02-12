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
#include "error.h"

dict_settings *get_settings( dict *d );
dict_methods *get_methods( dict *d );
rstat reconfigure( dict *d, dict_settings *settings );


#endif
