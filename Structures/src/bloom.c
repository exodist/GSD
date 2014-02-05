#include "bloom.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

bloom *bloom_create(size_t size, uint8_t k, bloom_hasher *bh, void *meta) {
    bloom *b = malloc(sizeof(bloom));
    if (!b) return NULL;

    assert(k < 5 && k > 1);

    b->k    = k;
    b->bh   = bh;
    b->meta = meta;
    b->size = size;
    b->bits = size * 8;

    b->bitmap = malloc(size);
    if (!b->bitmap) {
        free(b);
        return NULL;
    }
    memset(b->bitmap, 0, size);

    return b;
}

int bloom_insert(bloom *b, const void *item) {
    uint64_t  hash = b->bh(item, b->meta);
    uint32_t *nums = hash_to_nums(b->bits, hash, b->k);
    if (!nums) return -1;

    int out = bloom_do_insert(b, nums);
    free(nums);
    return out;
}

int bloom_lookup(bloom *b, const void *item) {
    uint64_t  hash = b->bh(item, b->meta);
    uint32_t *nums = hash_to_nums(b->bits, hash, b->k);
    if (!nums) return -1;

    int out = bloom_do_lookup(b, nums);
    free(nums);
    return out;
}

uint32_t *hash_to_nums(size_t bits, uint64_t hash, uint8_t k) {
    /*
        The 64 bit hash is divided into 4 16-bit hashes.
        h16[0] h16[1] h16[2] h16[3]
    */
    uint16_t *h16 = (void *)(&hash);

    uint32_t *nums = malloc(sizeof(uint32_t) * k);
    if (!nums) return NULL;
    uint16_t *nums3 = (uint16_t *)(nums + 3);

    /*
        If we have more bits available than a 16 bit hash can cover, we
        need more bits, so we divide the hash this way:

                0      1      2      3      0
              ------ ------ ------ ------ ------
        Hash1 h16[0] h16[1]
        Hash2               h16[2] h16[3]
        Hash3        h16[1] h16[2]
        Hash4                      h16[3] h16[0]

        This gives us 4 32-bit hashes that are at least sufficient for our
        purposes.
    */
    switch(k) {
        case 4:
            nums3[0] = h16[3];
            nums3[1] = h16[0];
        case 3:
            nums[2] = *((uint32_t *)(h16 + 1));
        case 2:
            nums[0] = *((uint32_t *)(h16 + 0));
            nums[1] = *((uint32_t *)(h16 + 2));
        break;

        default:
        assert(0);
    }

    return nums;
}

void bloom_locate(bloom *b, uint32_t num, size_t *byte, uint8_t *bit) {
    num   = num % b->bits;
    *byte = num / 8;
    *bit  = num % 8;
}

int bloom_do_insert(bloom *b, uint32_t *nums) {
    int out = 0;
    for( int i = 0; i < b->k; i++) {
        uint8_t bit;
        size_t  byte;
        bloom_locate(b, nums[i], &byte, &bit);
        assert( byte < b->size );
        assert( bit  < 8       );

        uint8_t val = 1 << bit;
        uint8_t prev = __atomic_fetch_or(b->bitmap + byte, val, __ATOMIC_ACQ_REL);

        if (val & prev) out++;
    }
    return out >= b->k ? 1 : 0;
}

int bloom_do_lookup(bloom *b, uint32_t *nums) {
    for( int i = 0; i < b->k; i++) {
        uint8_t bit;
        size_t  byte;
        bloom_locate(b, nums[i], &byte, &bit);

        uint8_t val = 1 << bit;
        uint8_t current = __atomic_load_n(b->bitmap + byte, __ATOMIC_CONSUME);

        if (val & bit) continue;

        return 0;
    }

    return 1;
}

