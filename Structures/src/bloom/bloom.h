#ifndef BLOOM_BLOOM_H
#define BLOOM_BLOOM_H

#include "../include/gsd_struct_bloom.h"

struct bloom {
    singularity *s;

    union {
        hasher  *bh;
        hasher **bhs;
    } hashers;

    size_t   size;
    size_t   bits;
    uint8_t *bitmap;
    uint8_t  k;
    uint8_t  is_multihash;
};

uint64_t *get_nums(bloom *b, const object *item);
uint64_t *hash_to_nums(size_t bits, uint64_t hash, uint8_t k);

int bloom_do_insert(bloom *b, uint64_t *nums);
int bloom_do_lookup(bloom *b, uint64_t *nums);

void bloom_locate(bloom *b, uint64_t num, size_t *byte, uint8_t *bit);

#endif
