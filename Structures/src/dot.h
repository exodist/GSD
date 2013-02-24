/* NOT PUBLIC API, SUBJECT TO CHANGE WITHOUT WARNING
 *
 * This header is to be used internally in GSD Dictionary code. It is not
 * intended to be used by programs that use GSD Dictionary.
 *
 * The code in this header can change at any time with no warning. You should
 * only use the include/gsd_dict.h header file in external programs.
\*/

#ifndef DOT_H
#define DOT_H

#include <stdarg.h>

#include "epoch.h"
#include "structure.h"
#include "node_list.h"
#include "error.h"

#define DOT_BUFFER_INC 1024

typedef struct dot dot;
struct dot {
    set      *set;
    dict_dot *decode;

    size_t first_slot;
    int first_slot_set;

    nlist *nl;
    dict *ref_tracker;
    size_t null_counter;

    char *epochs;
    char *slots;
    char *nodes;
    char *node_level;
    char *slot_level;
    char *refs;

    size_t epochs_size;
    size_t slots_size;
    size_t nodes_size;
    size_t node_level_size;
    size_t slot_level_size;
    size_t refs_size;

    size_t epochs_length;
    size_t slots_length;
    size_t nodes_length;
    size_t node_level_length;
    size_t slot_level_length;
    size_t refs_length;
};

// Also in include/gsd_dict.h
char *dump_dot( dict *d, dict_dot *decode );

char *dump_node_label( void *key, void *value );
char *do_dump_dot( dict *d, set *s, dict_dot decode );

rstat dot_print( char **buffer, size_t *size, size_t *length, char *format, va_list args );

rstat dot_print_epochs( dot *d, char *format, ... );
rstat dot_print_slots( dot *d, char *format, ... );
rstat dot_print_nodes( dot *d, char *format, ... );
rstat dot_print_node_level( dot *d, char *format, ... );
rstat dot_print_slot_level( dot *d, char *format, ... );
rstat dot_print_refs( dot *d, char *format, ... );

rstat dump_dot_epochs( dict *d, dot *dd );
rstat dump_dot_slots( set *s, dot *dd );
char *dump_dot_merge( dot *dd );

int dump_dot_ref_cmp( void *meta, void *key1, void *key2 );
size_t dump_dot_ref_loc( size_t slot_count, void *meta, void *key );
int dump_dot_ref_free_handler( void *key, void *value, void *args );
int dump_dot_ref_handler( void *key, void *value, void *args );

#endif
