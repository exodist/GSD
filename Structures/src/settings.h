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

// These are from include/gsd_dict.h
dict_settings *dict_get_settings( dict *d );
dict_methods *dict_get_methods( dict *d );
int dict_reconfigure( dict *d, dict_settings *settings );


#endif
