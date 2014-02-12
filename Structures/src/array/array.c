#include "array.h"
#include <string.h>

array_data *array_data_create(size_t s) {
    array_data *ad = malloc(sizeof(array_data));
    if (!ad) return NULL;
    memset(ad, 0, sizeof(ad));

    ad->size = s;

    ad->lookup = malloc(sizeof(uint32_t) * s);
    if (!ad->lookup) {
        free(ad);
        return NULL;
    }

    size_t storage_size = STORAGE_EXTRA + s * sizeof(sref *);
    ad->storage = malloc(storage_size);
    if (!ad->storage) {
        free(ad->lookup);
        free(ad);
        return NULL;
    }
    memset(ad->storage, 0, storage_size);

    ad->available = bitmap_create(STORAGE_EXTRA + s);
    if (!ad->available) {
        free(ad->storage);
        free(ad->lookup);
        free(ad);
        return NULL;
    }

    return ad;
}

void array_data_free(array_data *ad) {
    free(ad->available);
    free(ad->storage);
    free(ad->lookup);
    free(ad);
}

array *array_create(size_t init_size, size_t grow_delta, refdelta *rd) {
    array *a = malloc(sizeof(array));
    if (!a) return NULL;
    memset(a, 0, sizeof(array));

    a->current = array_data_create(init_size);
    if (!a->current) {
        free(a);
        return NULL;
    }

    a->p = build_prm(5, 63, 100);
    if (!a->p) {
        array_data_free(a->current);
        free(a);
        return NULL;
    }

    a->delta = rd;
    a->grow  = grow_delta;

    return a;
}

result array_get(array *a, int64_t idx) {
    //get offset
    //get lookup at offset
    //get item from storage using lookup
    //verify offset is unchanged, or try again

    //return item
}

result array_get_ref(array *a, int64_t idx) {
}


result array_delete(array *a, int64_t idx) {
}

result array_deref(array *a, int64_t idx) {
}


result array_set(array *a, int64_t idx, void *val) {
}

result array_update(array *a, int64_t idx, void *val) {
}

result array_insert(array *a, int64_t idx, void *val) {
}

result array_set_ref(array *a, int64_t idx, ref *val) {
}

result array_update_ref(array *a, int64_t idx, ref *val) {
}

result array_insert_ref(array *a, int64_t idx, ref *val) {
}


result array_pop(array *a) {
}

result array_shift(array *a) {
}

result array_pop_ref(array *a) {
}

result array_shift_ref(array *a) {
}


// Returns index, or -1 on failure.
int64_t array_push(array *a, void *val) {
}

int64_t array_unshift(array *a, void *val) {
}

int64_t array_push_ref(array *a, ref *val) {
}

int64_t array_unshift_ref(array *a, ref *val) {
}

result array_cmp_update(array *a, int64_t idx, void *old, void *new) {
}

result array_cmp_update_ref(array *a, int64_t idx, ref *old, ref *new) {
}

result array_cmp_val_delete(array *a, int64_t idx, void *old, void *new) {
}

result array_cmp_ref_delete(array *a, int64_t idx, ref *old, void *new) {
}

result array_cmp_val_deref(array *a, int64_t idx, void *old, ref *new) {
}

result array_cmp_ref_deref(array *a, int64_t idx, ref *old, ref *new) {
}

