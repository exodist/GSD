#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"
#include "util.h"
#include "structures.h"
#include "balance.h"
#include "location.h"
#include "alloc.h"
#include "epoch.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

uint8_t max_bit( uint64_t num ) {
    uint8_t bit = 0;
    while ( num > 0 ) {
        num >>= 1;
        bit++;
    }

    return bit;
}


