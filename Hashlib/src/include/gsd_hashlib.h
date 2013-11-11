#ifndef GSD_HASHLIB_H
#define GSD_HASHLIB_H

#include <stdint.h>
#include <stdlib.h>

// All hash functions should fit this typedef so that they can be used
// interchangibly
// when state is null the seed will be used, when state is not null the value
// pointed at will be used.
typedef uint64_t (hash_function)(uint8_t *data, size_t length, const uint64_t *state);

uint64_t fnv_hash(uint8_t *data, size_t length, const uint64_t *state);

#endif
