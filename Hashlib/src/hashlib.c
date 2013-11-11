#include "include/gsd_hashlib.h"
#include "hashlib.h"

uint64_t fnv_hash(uint8_t *data, size_t length, const uint64_t *state) {
    uint64_t key = state ? *state : FNV_SEED;

    if ( length < 1 ) return key;

    for ( size_t i = 0; i < length; i++ ) {
        key ^= data[i];
        key *= FNV_PRIME;
    }

    return key;
}

