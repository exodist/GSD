#include "../include/gsd_struct_bitmap.h"
#include <stdio.h>
#include <assert.h>

int main() {
    bitmap *b = bitmap_create(1024);

    assert( bitmap_get(b, 0) == 0 );
    assert( bitmap_get(b, 1) == 0 );
    assert( bitmap_get(b, 2) == 0 );

    assert( bitmap_set(b, 2, 1) == 0 );
    assert( bitmap_get(b, 1) == 0 );
    assert( bitmap_get(b, 2) == 1 );
    assert( bitmap_get(b, 3) == 0 );

    assert( bitmap_fetch(b, 1, 0) == 0 );
    assert( bitmap_get(b, 0) == 1 );

    assert( bitmap_fetch(b, 1, 0) == 1 );
    assert( bitmap_get(b, 1) == 1 );

    assert( bitmap_fetch(b, 1, 0) == 3 );
    assert( bitmap_get(b, 3) == 1 );

    assert( bitmap_fetch(b, 1, 0) == 4 );
    assert( bitmap_get(b, 4) == 1 );

    bitmap *b2 = bitmap_clone(b, 6);
    assert( bitmap_get(b2, 0) == 1 );
    assert( bitmap_get(b2, 1) == 1 );
    assert( bitmap_get(b2, 2) == 1 );
    assert( bitmap_get(b2, 3) == 1 );
    assert( bitmap_get(b2, 4) == 1 );
    assert( bitmap_get(b2, 5) == 0 );

    assert( bitmap_fetch(b2, 1, 0) == 5 );
    assert( bitmap_fetch(b2, 1, 0) == -1 );

    bitmap *b3 = bitmap_clone(b2, 1024);
    assert( bitmap_fetch(b3, 1, 0) == 6 );

    assert( bitmap_get(b3, 1000) == 0 );

    bitmap *b4 = bitmap_create(1024);
    for (int i = 0; i < 1024; i++) {
        assert( bitmap_get(b4, i)   == 0 );
        assert( bitmap_fetch(b4, 1, 0) == i );
        assert( bitmap_get(b4, i)   == 1 );
    }
    for (int i = 0; i < 1024; i++) {
        assert( bitmap_get(b4, i)   == 1 );
        assert( bitmap_fetch(b4, 0, 0) == i );
        assert( bitmap_get(b4, i)   == 0 );
    }

    assert( bitmap_fetch(b4, 0, 0) == -1 );
    assert( bitmap_fetch(b4, 1, 4) == 0 );
    assert( bitmap_fetch(b4, 1, 4) == 1 );
    assert( bitmap_fetch(b4, 1, 4) == 2 );
    assert( bitmap_fetch(b4, 1, 4) == 3 );
    assert( bitmap_fetch(b4, 1, 4) == 4 );
    assert( bitmap_fetch(b4, 1, 4) == -1 );

    bitmap_free(b);
    bitmap_free(b2);
    bitmap_free(b3);
    bitmap_free(b4);

    printf( "All Done!\n" );
}
