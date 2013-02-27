/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef MERGE_H
#define MERGE_H

#include "error.h"

// These are also in include/gsd_dict.h
rstat merge( dict *from, dict *to );
rstat merge_refs( dict *from, dict *to );

#endif
