#include "parser.h";

scope *parse_token_set(token_set *ts, get_term *gt, dict *symbols, uint8_t *filename, size_t start_line, parse_error *e) {
    scope *sc = malloc(sizeof(scope));
    if (!sc) return NULL;
    memset(sc, 0, sizeof(sc));

    sc->symbols    = symbols;
    sc->filename   = filename;
    sc->start_line = start_line;
    sc->end_line   = start_line;

    token_set_iterator *tsi = iterate_token_set(ts);
    while( token *t = ts_iter_next(tsi) ) {
        if (t->type == TOKEN_INVALID) {
            // Report Error
            free_scope( sc );
            return NULL;
        }

        if (t->type == TOKEN_CONTROL) {
            // Report Error
            free_scope( sc );
            return NULL;
        }

        if (t->type == TOKEN_NEWLINE) {
            push_line( sc, 1 );
            continue;
        }

        if (t->type == TOKEN_TERM) {
            push_statement( sc );
            continue;
        }

        if (t->type == TOKEN_SPACE) {
            // push space node
            continue;
        }

        // resolve
        resolve_status ok = resolve_symbol(sc, t, &sv);
        switch(ok) {
            case RESOLVE_FAIL:
                // Report resolution error
                free_scope( sc );
                return NULL;

            case RESOLVE_KEY:
                // Process keyword
                break;

            case RESOLVE_SYM:
            case RESOLVE_STATE:
                //push the ref
                break;

            case RESOLVE_LEX:
            case RESOLVE_BUILD:
                // ensure token type is symbol, or combine
                // push the token
                break;
        }
    }
}

int push_node( scope *sc, node *n ) {

}

int push_statement( scope *sc ) {
    // Reset the 'term' dict
}

int push_line( scope *sc, size_t count ) {

}

node *create_node( int type, void *item ) {

}

resolve_status resolve_symbol( scope *sc, token *t, void **sym );
    // Inject any 'term' into the term dict
}

void free_scope(scope *sc) {

}
