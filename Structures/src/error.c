#include "include/gsd_dict_return.h"
#include "error.h"
#include "devtools.h"

const char *dict_stat_message( dict_stat s ) {
    return error_message( s );
}

const char *error_message( rstat s ) {
    return s.bit.message;
}

rstat make_error( uint8_t fail, uint8_t rebal, uint8_t cat, const char *msg, uint8_t invalid, size_t ln, const char *fn ) {
    rstat st = { .bit = {fail, rebal, cat, invalid, msg, ln, fn }};
    return st;
}

rstat rstat_ok    = { .bit = {0, 0, DICT_NO_ERROR,      0, NULL,            0, "UNKNOWN" }};
rstat rstat_mem   = { .bit = {1, 0, DICT_OUT_OF_MEMORY, 0, "Out of memory", 0, "UNKNOWN" }};
rstat rstat_trans = { .bit = {1, 0, DICT_NO_ERROR,      0, NULL,            0, "UNKNOWN" }};
rstat rstat_trigg = { .bit = {1, 0, DICT_TRIGGER,       0, "Trigger Error", 0, "UNKNOWN" }};

rstat rstat_patho = { .bit = {
    0, 0, DICT_PATHOLOGICAL, 0,
    "Pathological Data detected, rebalance operations suspended for some slots",
    0, "UNKNOWN"
}};

rstat rstat_unimp = { .bit = {
    1, 0, DICT_UNIMPLEMENTED, 0,
    "Function not implemented",
    0, "UNKNOWN"
}};

rstat rstat_imute = { .bit = {
    1, 0, DICT_IMMUTABLE, 0,
    "Attempt to modify Immutable dictionary",
    0, "UNKNOWN"
}};


