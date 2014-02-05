#ifndef BLOOM_H
#define BLOOM_H

#include "include/gsd_struct_bloom.h"

struct bloom {
    bloom_hasher *bh;

    void *meta;

    size_t   size;
    size_t   bits;
    uint8_t *bitmap;
    uint8_t  k;
};

uint32_t *hash_to_nums(size_t bits, uint64_t hash, uint8_t k);

int bloom_do_insert(bloom *b, uint32_t *nums);
int bloom_do_lookup(bloom *b, uint32_t *nums);

void bloom_locate(bloom *b, uint32_t num, size_t *byte, uint8_t *mask);

#endif
