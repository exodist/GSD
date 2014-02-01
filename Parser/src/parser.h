#include "include/gsd_parser_api.h"

typedef enum {
    RESOLVE_FAIL  = 0,
    RESOLVE_KEY   = 1,
    RESOLVE_SYM   = 2,
    RESOLVE_STATE = 3,
    RESOLVE_LEX   = 4,
    RESOLVE_BUILD = 5,
} resolve_status;

resolve_status resolve_symbol( scope *sc, token *t, void **sym );

int push_node( scope *sc, node *n );
int push_statement( scope *sc );
int push_line( scope *sc, size_t count );
node *create_node( int type, void *item );
