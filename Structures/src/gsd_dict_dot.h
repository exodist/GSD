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
#include <stdarg.h>

#define DOT_BUFFER_INC 1024

typedef struct nl nl;
struct nl {
    node  *node;
    nl    *next;
    size_t nid;
};

typedef struct dot dot;
struct dot {
    set      *set;
    dict_dot *decode;

    size_t first_slot;
    int first_slot_set;

    nl *nl_end;
    nl *nl_start;
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

char *dict_dump_node_label( void *key, void *value );
char *dict_do_dump_dot( dict *d, set *s, dict_dot decode );

int dict_dot_print( char **buffer, size_t *size, size_t *length, char *format, va_list args );

int dict_dot_print_epochs( dot *d, char *format, ... );
int dict_dot_print_slots( dot *d, char *format, ... );
int dict_dot_print_nodes( dot *d, char *format, ... );
int dict_dot_print_node_level( dot *d, char *format, ... );
int dict_dot_print_slot_level( dot *d, char *format, ... );
int dict_dot_print_refs( dot *d, char *format, ... );

int dict_dump_dot_epochs( dict *d, dot *dd );
int dict_dump_dot_slots( dict *d, dot *dd );
char *dict_dump_dot_merge( dot *dd );

int dict_dump_dot_ref_cmp( dict_settings *s, void *key1, void *key2 );
size_t dict_dump_dot_ref_loc( dict_settings *s, void *key );
int dict_dump_dot_ref_free_handler( void *key, void *value, void *args );
int dict_dump_dot_ref_handler( void *key, void *value, void *args );

#endif
