/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the gsd_dict_api.h header file in external programs.
\*/

#ifndef GSD_DICT_DOT_H
#define GSD_DICT_DOT_H

#include "gsd_dict_api.h"
#include "gsd_dict_structures.h"

#define DOT_BUFFER_SIZE 256

int dict_dump_dot_start( dot *dt );
int dict_dump_dot_slink( dot *dt, int s1, int s2 );
int dict_dump_dot_subgraph( dot *dt, int s, node *n );
int dict_dump_dot_node( dot *dt, char *line, node *n, char *label );
int dict_dump_dot_write( dot *dt, char *add );

#endif
