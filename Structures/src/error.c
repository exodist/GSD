#include <assert.h>

#include "include/gsd_dict_return.h"
#include "error.h"

#define MESSAGE_COUNT 12

char *error_messages[MESSAGE_COUNT] = {
    /* 0  */ "Success",
    /* 1  */ "Out of memory",
    /* 2  */ "Pathological Data detected",
    /* 3  */ "Function not implemented",
    /* 4  */ "Unknown error",
    /* 5  */ "Settings may not be NULL.",
    /* 6  */ "Methods may not be NULL.",
    /* 7  */ "The 'cmp' method may not be NULL.",
    /* 8  */ "The 'loc' method may not be NULL.",
    /* 9  */ "The epoch limit cannot be less than 4 (except for 0).",
    /* 10 */ "The Compare method must return 1, 0, or -1",
    /* 11 */ "NULL can not be used as the 'old value' in compare and swap"
};

char *dict_stat_message( dict_stat *s ) {
    return error_message( s );
}

char *error_message( rstat *s ) {
    assert( s->bit.message_idx < MESSAGE_COUNT );
    return error_messages[s->bit.message_idx];
}

rstat error( uint8_t fail, uint8_t rebal, uint8_t cat, uint8_t midx ) {
    assert( midx < MESSAGE_COUNT );
    rstat st = { .bit = {fail, rebal, cat, midx} };
    return st;
}

rstat rstat_ok    = { .bit = {0, 0, DICT_NO_ERROR,      0} };
rstat rstat_mem   = { .bit = {1, 0, DICT_OUT_OF_MEMORY, 1} };
rstat rstat_trans = { .bit = {1, 0, DICT_NO_ERROR,      0} };
rstat rstat_patho = { .bit = {0, 0, DICT_PATHOLOGICAL,  2} };
rstat rstat_unimp = { .bit = {1, 0, DICT_UNIMPLEMENTED, 3} };

