#include "include/gsd_string_dict.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


uint8_t *copy( char *s ) {
    size_t len = strlen(s);
    uint8_t *out = malloc( len + 1 );
    if (!out) return NULL;
    memcpy( out, s, len + 1 );
    return out;
}

int main() {
    dict *d = new_string_dict( 128, free );
    assert(d);

    dict_stat ds = strd_set( d, "foo", copy("bar") );
    assert( !ds.num );

    get_stat gs = strd_get( d, "foo" );
    assert( !gs.stat.num );
    assert( gs.got );
    assert( !memcmp( gs.got, "bar", 4 ));

    gs = strd_get( d, "fake" );
    assert( !gs.stat.num );
    assert( !gs.got );

    ds = strd_insert( d, "foo", copy("baz") );
    assert( ds.bit.fail );

    gs = strd_get( d, "foo" );
    assert( !gs.stat.num );
    assert( gs.got );
    assert( !memcmp( gs.got, "bar", 4 ));

    ds = strd_update( d, "foo", copy("baz") );
    assert( !ds.bit.fail );

    gs = strd_get( d, "foo" );
    assert( !gs.stat.num );
    assert( gs.got );
    assert( !memcmp( gs.got, "baz", 4 ));

    ds = strd_delete( d, "foo" );
    assert( !ds.bit.fail );
    gs = strd_get( d, "foo" );
    assert( !gs.stat.num );
    assert( !gs.got );

    // One more to make sure it is collected
    ds = strd_set( d, "foo", copy("bar") );
    assert( !ds.num );

    dict_free(&d);
    printf( "\n\nAll tests passed\n" );
}
