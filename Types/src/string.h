#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stdlib.h>
#include "include/type_api.h"
#include "include/string_api.h"
#include "structures.h"
#include <stdlib.h>
#include <unistr.h>

const uint8_t *iterator_next_part( string_iterator *i, ucs4_t *c );

void free_string( object *o );

#endif
