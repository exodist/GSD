#include <stdint.h>

#include "util.h"

uint8_t max_bit( uint64_t num ) {
    uint8_t bit = 0;
    while ( num > 0 ) {
        num >>= 1;
        bit++;
    }

    return bit;
}


