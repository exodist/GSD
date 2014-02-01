#include <stdlib.h>
#include <stdint.h>
#include "../../../Structures/src/include/gsd_string_dict.h"
#include "../../../Tokenizer/src/include/gsd_tokenizer_api.h"

typedef struct node      node;
typedef struct statement statement;
typedef struct scope     scope;
typedef struct quote     quote;
typedef struct stashval  stashval;

typedef struct parse_error parse_error;

typedef struct keyword             keyword;
typedef struct keyword_wordquote   keyword_wordquote;
typedef struct keyword_begin       keyword_begin;
typedef struct keyword_declare     keyword_declare;
typedef struct keyword_scope       keyword_scope;
typedef struct keyword_builder     keyword_builder;
typedef struct keyword_conditional keyword_conditional;
typedef struct keyword_turnary     keyword_turnary;
typedef struct keyword_quote       keyword_quote;
typedef struct keyword_delimited   keyword_delimited;

typedef int(callback)(token_set_iterator *i, keyword *kw, scope *sc, statement *st, void *arg);
typedef uint8_t *(get_term)(symbol *s, dict *symbols, dict *lexicals, dict *states, dict *builder);

struct parse_error {
    enum {
        ERROR_NONE = 0,
        ERROR_INVALID_TOKEN,
        ERROR_RESOLVING_TOKEN,
        ERROR_CONTROL_TOKEN,
    } type;

    node  *start;
    token *token;

    size_t line_number;
    uint8_t *filename;
};

struct stashval {
    union { void *ext, keyword *kw } item;
    enum  { STASH_EXT, STASH_KW    } type;
};

struct quote {
    uint8_t *string;
    size_t   size;
    size_t   start_line;
    size_t   end_line;
};

struct node {
    enum { NODE_SCOPE, NODE_QUOTE, NODE_SYMBOL, NODE_REF, NODE_SPACE } type;
    union {
        scope *scope;
        quote *quote;
        token *symbol;
        void  *ref;
    } item;

    node *left;
    node *right;

    node *next; // For internal use
};

struct statement {
    node *start;

    size_t start_line;
    size_t end_line;
};

struct scope {
    // Holds stashval objects
    dict *symbols; // imports, and closed-over imports

    // Regular dictionaries
    dict *states;       // Defined here
    dict *state_closed; // Inherited

    // These are all true/false
    dict *lexicals;   // Defined here
    dict *lex_closed; // Inherited
    dict *builder;    // meta

    // Set to 0 to return from itself
    // Set to 1 to return the parent scope: if (X) { return }
    // This is used for scopes that are not directly run on their own, such as
    // conditionals.
    uint8_t return_level;

    uint8_t *filename;
    size_t start_line;
    size_t end_line;

    statement **statements;
    size_t      statements_size;
    size_t      statement_idx;

    dict *term;
};

struct keyword {
    enum {
        KW_SCOPE,       // scopes, subs, etc { ... }
        KW_BUILDER,     // declare a scope, like func foo() { ... } and class NAME { ... }
        KW_DECLARE,     // declare variables
        KW_CONDITIONAL, // if/while/for/etc.
        KW_TURNARY,     // ... ? ... : ...
        KW_WORDQUOTE,   // foo ~> bar, Some::Package, foo.bar foo:bar foo:[1] foo:{bar}
        KW_QUOTE,       // "...", '...', `...`
        KW_DELIMITED,   // qw/.../ qr/.../ s/.../.../.. m/.../..
        KW_BEGIN,       // BEGIN { ... }
    } type;

    callback *kw_cb;
    void     *kw_cb_arg;
};

struct keyword_begin   { keyword keyword; };
struct keyword_declare { keyword keyword; };

struct keyword_wordquote {
    keyword keyword;
    uint8_t *terminate;
}

struct keyword_scope {
    keyword keyword;
    dict *symbols;
    uint8_t *terminate;
};

struct keyword_builder {
    keyword keyword;

    dict *symbols;

    enum { NAME_REQUIRED, NAME_OPTIONAL, NAME_FORBIDDEN } name;
    enum { NT_IDENTIFIER, NT_STRING, NT_BOTH } name_type;

    enum { PARAM_REQUIRED, PARAM_OPTIONAL, PARAM_FORBIDDEN } param;
    enum { PT_SIGNAURE, PT_STATEMENT, PT_TEXT } param_type;
};

struct keyword_conditional {
    keyword keyword;

    // All \0 terminated
    uint8_t *branch;
    uint8_t *final;
    uint8_t *name; // Default

    void    *type; // Default

    uint8_t postfixable;
    enum { CV_STATEMENT, CV_VARIABLE, CV_COMBO } validate;
};

struct keyword_turnary {
    keyword keyword;
    uint8_t *divide;
};

struct keyword_quote {
    keyword keyword;
    uint8_t *terminate;
    uint8_t *escape;
};

struct keyword_delimited {
    keyword keyword;
    size_t sections;
    uint8_t *escape;
    uint8_t take_flags;
};

scope *parse_token_set(token_set *ts, get_term *gt, dict *symbols, uint8_t *filename, size_t start_line, parse_error *e);

void free_scope(scope *sc);
