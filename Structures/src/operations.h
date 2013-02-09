/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef OPERATIONS_H
#define OPERATIONS_H

int dict_do_set( dict *d, void *key, void *old_val, void *val, int override, int create, location **locator );

int dict_do_deref( dict *d, void *key, location *loc, sref *swap );

#endif
