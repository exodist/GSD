#include "include/gsd_structures.h"
#include "error.h"
#include "devtools.h"

const char *dict_stat_message( dict_stat s ) {
    return error_message( s );
}

const char *error_message( rstat s ) {
    return s.bit.message;
}

rstat make_error( uint8_t fail, uint8_t rebal, uint8_t cat, const char *msg, size_t ln, const char *fn ) {
    rstat st = { .bit = {fail, rebal, cat, msg, ln, fn }};
    return st;
}

rstat rstat_ok    = { .bit = {0, 0, DICT_NO_ERROR,      NULL,            0, "UNKNOWN" }};
rstat rstat_mem   = { .bit = {1, 0, DICT_OUT_OF_MEMORY, "Out of memory", 0, "UNKNOWN" }};
rstat rstat_trans = { .bit = {1, 0, DICT_NO_ERROR,      NULL,            0, "UNKNOWN" }};
rstat rstat_trigg = { .bit = {1, 0, DICT_TRIGGER,       "Trigger Error", 0, "UNKNOWN" }};

rstat rstat_patho = { .bit = {
    0, 0, DICT_PATHOLOGICAL,
    "Pathological Data detected, rebalance operations suspended for some slots",
    0, "UNKNOWN"
}};

rstat rstat_unimp = { .bit = {
    1, 0, DICT_UNIMPLEMENTED,
    "Function not implemented",
    0, "UNKNOWN"
}};

rstat rstat_imute = { .bit = {
    1, 0, DICT_IMMUTABLE,
    "Attempt to modify Immutable dictionary",
    0, "UNKNOWN"
}};


