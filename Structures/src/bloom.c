#include "bloom.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

bloom *bloom_create_k(void *meta, size_t size, uint8_t k, ...) {
    bloom *out = bloom_create(meta, size, k, NULL);

    out->hashers.bhs = malloc(sizeof(hasher *) * k);
    if (!out->hashers.bhs) {
        free_bloom(out);
        return NULL;
    }

    out->is_multihash = 1;

    va_list ks;
    va_start(ks, k);
    for(int i = 0; i < k; i++) {
        out->hashers.bhs[i] = va_arg(ks, hasher *);
    }
    va_end(ks);

    return out;
}

bloom *bloom_create(void *meta, size_t size, uint8_t k, hasher *bh) {
    assert(k > 0);
    bloom *b = malloc(sizeof(bloom));
    if (!b) return NULL;

    b->k      = k;
    b->meta   = meta;
    b->size   = size;
    b->bits   = size * 8;
    b->is_multihash = 0;

    b->hashers.bh = bh;

    b->bitmap = malloc(size);
    if (!b->bitmap) {
        free(b);
        return NULL;
    }
    memset(b->bitmap, 0, size);

    return b;
}

uint64_t *get_nums(bloom *b, const void *item) {
    uint64_t *nums = NULL;

    if (b->is_multihash) {
        nums = malloc(sizeof(uint64_t) * b->k);
        if (!nums) return NULL;
        for (int i = 0; i < b->k; i++) {
            nums[i] = b->hashers.bhs[i](item, b->meta);
        }
    }
    else {
        uint64_t hash = b->hashers.bh(item, b->meta);
        nums = hash_to_nums(b->bits, hash, b->k);
    }

    return nums;
}

int bloom_insert(bloom *b, const void *item) {
    uint64_t *nums = get_nums(b, item);
    if (!nums) return -1;

    int out = bloom_do_insert(b, nums);
    free(nums);
    return out;
}

int bloom_lookup(bloom *b, const void *item) {
    uint64_t *nums = get_nums(b, item);
    if (!nums) return -1;

    int out = bloom_do_lookup(b, nums);
    free(nums);
    return out;
}

uint64_t *hash_to_nums(size_t bits, uint64_t hash, uint8_t k) {
    /*
        The 64 bit hash is divided into 4 16-bit hashes.
        h16[0] h16[1] h16[2] h16[3]
    */
    uint8_t *h8 = (void *)(&hash);

    uint64_t *nums = malloc(sizeof(uint64_t) * k);
    if (!nums) return NULL;
    uint8_t *nums8 = (void *)nums;

    /*
                0     1     2     3     4     5     6     7     0     1     2     3     4     5     6     7
              ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- -----
        Hash1 h8[0] h8[1] h8[2] h8[3]
        Hash2                         h8[4] h8[5] h8[6] h8[7]
        Hash3             h8[2] h8[3] h8[4] h8[5]
        Hash4       h8[1] h8[2] h8[3] h8[4]

        Hash5                   h8[3] h8[4] h8[5] h8[6]
        Hash6                               h8[5] h8[6] h8[7] h8[0]
        Hash7                                     h8[6] h8[7] h8[0] h8[1]
        Hash8                                           h8[7] h8[0] h8[1] h8[2]

        This gives us 4 32-bit hashes that are at least sufficient for our
        purposes.
    */
    switch(k) {
        default:
        case 10:
            nums8[9 * 8 + 0] = h8[1];
            nums8[9 * 8 + 1] = h8[3];
            nums8[9 * 8 + 2] = h8[5];
            nums8[9 * 8 + 3] = h8[7];
        case 9:
            nums8[8 * 8 + 0] = h8[0];
            nums8[8 * 8 + 1] = h8[2];
            nums8[8 * 8 + 2] = h8[4];
            nums8[8 * 8 + 3] = h8[6];
        case 8:
            nums8[7 * 8 + 0] = h8[7];
            nums8[7 * 8 + 1] = h8[0];
            nums8[7 * 8 + 2] = h8[1];
            nums8[7 * 8 + 3] = h8[2];
        case 7:
            nums8[6 * 8 + 0] = h8[6];
            nums8[6 * 8 + 1] = h8[7];
            nums8[6 * 8 + 2] = h8[0];
            nums8[6 * 8 + 3] = h8[1];
        case 6:
            nums8[5 * 8 + 0] = h8[5];
            nums8[5 * 8 + 1] = h8[6];
            nums8[5 * 8 + 2] = h8[7];
            nums8[5 * 8 + 3] = h8[0];
        case 5:
            nums[4] = *((uint32_t *)(h8 + 3));
        case 4:
            nums[3] = *((uint32_t *)(h8 + 1));
        case 3:
            nums[2] = *((uint32_t *)(h8 + 2));
        case 2:
            nums[1] = *((uint32_t *)(h8 + 4));
        case 1:
            nums[0] = *((uint32_t *)(h8 + 0));
        break;
    }

    return nums;
}

void bloom_locate(bloom *b, uint64_t num, size_t *byte, uint8_t *bit) {
    num   = num % b->bits;
    *byte = num / 8;
    *bit  = num % 8;
}

int bloom_do_insert(bloom *b, uint64_t *nums) {
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

int bloom_do_lookup(bloom *b, uint64_t *nums) {
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

void free_bloom(bloom *b) {
    if (b->is_multihash) free(b->hashers.bhs);
    free(b->bitmap);
    free(b);
}
