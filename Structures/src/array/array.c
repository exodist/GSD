#include "array.h"

array *array_create(size_t init_size, size_t grow_delta, refdelta *rd);

result array_get    (array *a, int64_t idx);
result array_get_ref(array *a, int64_t idx);


