#include "bitmap.h"
#include <assert.h>
#include <string.h>

uint64_t FULL64 = 0xFFFFFFFFFFFFFFFFUL;
uint32_t FULL32 = 0xFFFFFFFFU;
uint32_t FULL16 = 0xFFFFU;
uint32_t FULL08 = 0xFFU;

bitmap *bitmap_alloc(int64_t bits) {
    assert(bits > 0);
    bitmap *b = malloc(sizeof(bitmap));
    if (!b) return NULL;
    b->bits = bits;

    int64_t bytes = bits / 64 * 8;
    if (bits % 8) bytes += 8;
    assert(bytes > 0);

    b->bytes = bytes;

    b->data = malloc(bytes);
    if (!b) {
        free(b);
        return NULL;
    }
    memset(b->data, 0, b->bytes);

    return b;
}

bitmap *bitmap_create(int64_t bits) {
    bitmap *b = bitmap_alloc(bits);
    if (!b) return NULL;
    return b;
}

bitmap *bitmap_clone(bitmap *o, int64_t bits) {
    bitmap *b = bitmap_alloc(bits);
    if (!b) return NULL;

    if (o->bytes < b->bytes) { // Growing
        memcpy(b->data, o->data, o->bytes);
    }
    else { // Same or shrinking
        memcpy(b->data, o->data, b->bytes);
    }

    return b;
}

int bitmap_get(bitmap *b, int64_t idx) {
    int64_t  slot = idx / 64;
    uint8_t  bit  = idx % 64;
    uint64_t mask = 1UL << bit;

    uint64_t state = __atomic_load_n(b->data + slot, __ATOMIC_CONSUME);
    return mask & state ? 1 : 0;
}

int bitmap_set(bitmap *b, int64_t idx, int state) {
    int64_t  slot = idx / 64;
    uint8_t  bit  = idx % 64;
    uint64_t mask = 1UL << bit;

    uint64_t old;

    if (state) {
        old = __atomic_fetch_or(b->data + slot, mask, __ATOMIC_ACQ_REL);
    }
    else {
        old = __atomic_fetch_and(b->data + slot, ~mask, __ATOMIC_ACQ_REL);
    }

    return mask & old;
}

int64_t bitmap_fetch(bitmap *b, int state) {
    for(int64_t i64 = 0; i64 < b->bytes; i64++) {
        while(1) {
            uint64_t current = __atomic_load_n(b->data + i64, __ATOMIC_ACQUIRE);
            if(state  && current == FULL64) break;
            if(!state && current == 0     ) break;

            int i32, i16, i08, i01;

            uint8_t   val;
            uint8_t  *parts08 = NULL;
            uint16_t *parts16 = NULL;
            uint32_t *parts32 = NULL;

            parts32 = (uint32_t *)&current;
            for (i32 = 0; i32 < 2; i32++) {
                int full = parts32[i32] == FULL32 ? 1 : 0;
                if (full          &&  state) continue;
                if (!parts32[i32] && !state) continue;

                parts16 = (uint16_t *)(parts32 + i32);
                break;
            }
            assert(parts16);

            for (i16 = 0; i16 < 2; i16++) {
                int full = parts16[i16] == FULL16 ? 1 : 0;
                if (full          &&  state) continue;
                if (!parts16[i16] && !state) continue;

                parts08 = (uint8_t *)(parts16 + i16);
                break;
            }
            assert(parts08);

            for (i08 = 0; i08 < 2; i08++) {
                int full = parts08[i08] == FULL08 ? 1 : 0;
                if (full          &&  state) continue;
                if (!parts08[i08] && !state) continue;

                val = parts08[i08];
                break;
            }

            for (i01 = 0; i01 < 8; i01++) {
                uint8_t mask = 1 << i01;
                int taken = val & mask;
                if(taken  &&  state) continue;
                if(!taken && !state) continue;
                break;
            }

            int64_t idx = i64 * 64 + i32 * 32 + i16 * 16 + i08 * 8 + i01;
            if (idx >= b->bits) return -1;

            int64_t  slot = idx / 64;
            uint8_t  bit  = idx % 64;
            uint64_t mask = 1UL << bit;

            int ok = __atomic_compare_exchange_n(
                b->data + slot,
                &current,
                state ? (current | mask) : (current & ~mask),
                0,
                __ATOMIC_ACQ_REL,
                __ATOMIC_RELAXED
            );

            if (ok) return idx;
        }
    }

    return -1;
}

void bitmap_free(bitmap *b) {
    free(b->data);
    free(b);
}


