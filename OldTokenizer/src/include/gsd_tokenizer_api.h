#ifndef GSD_TOKENIZER_API_H
#define GSD_TOKENIZER_API_H

#include <stdlib.h>
#include <stdint.h>

typedef struct grammar grammar;
typedef struct error   error;
typedef struct match   match;

typedef struct scope_match scope_match;
typedef struct token_match token_match;
typedef struct group_match group_match;

typedef struct match_result match_result;

// Check should return NULL if the token_match is not a keyword. When
// token_match is a keyword it should return a pattern.
typedef uint8_t *(keyword_check)(token_match *m, void *key_arg, void *scope_meta);

// Match will include the keyword itself, as well as the match from the keyword
// pattern. The result should contain a match list to insert into the parent
// match, or NULL to not put anything into the parent. If there is an error it
// should set the error.
typedef match_result (keyword_run)(match *m, void *key_arg, void *scope_meta);

struct error {
    size_t  line;
    size_t  place;
    uint8_t message[256];
};

struct match_result {
    scope_match *match;
    int   has_error;
    error error;
};

struct match {
    enum { MATCH_TOKEN, MATCH_SCOPE, MATCH_GROUP } type;
    size_t line_number;
    size_t start_char;
    size_t length;
    match *next;
};

struct scope_match {
    match match;
    void *meta;
    void *keywords;
    match *children;
};

struct token_match {
    match match;
    uint8_t *value;
    size_t   length;

    // Will be name of pattern matched, or name of structure in the pattern
    uint8_t *matched;

    uint8_t space_prefix;
    uint8_t space_postfix;
};

struct group_match {
    token_match token_match;

    match **parts;
    size_t  part_count;
};

// Create a grammar, optionally you can give it callbacks for identifying and
// running keywords.
grammar *new_grammar( keyword_check *kc, void *key_arg, keyword_run *kr );

// Returns null on success, an error message on failure
// name must be unique for any item added to a grammar
error add_pattern( grammar *g, uint8_t *name, uint8_t *pattern );
error add_grammer( grammar *g, uint8_t *name, grammar *child   );

// Match string against pattern 'name' in the grammar
match_result match_pattern( grammar *g, uint8_t *name, uint8_t *string );

// returns key_arg so that you can free it if needed
void *free_grammar( grammar *g );

// Free a match structure, the free_meta callback is used to free the
// scope-meta objects. If free_meta is NULL then the scope-meta objects will be
// ignored.
void free_match( match *m, void(*free_meta)(void *) );

#endif
