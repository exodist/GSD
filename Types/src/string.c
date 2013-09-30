#include "include/string_api.h"
#include "include/static_api.h"
#include "include/object_api.h"
#include "include/collector_api.h"
#include "string.h"
#include "structures.h"
#include <assert.h>
#include <string.h>

size_t string_bytes( object *s ) {
    assert( s->ref_count );
    assert( s->type >= STRING_TYPE_START );

    object_simple *ss = (object_simple*)s;
    switch (s->type) {
        case GC_STRING:
        case GC_ROPE:
        case GC_STRINGC:
        return ((string_header *)ss->simple_data.ptr)->bytes;

        case GC_SNIP:
        return ss->simple_data.snip.bytes;
    }

    // This will never happen, but the compiler can't tell.
    assert(0);
}

size_t string_chars( object *s ) {
    assert( s->ref_count );
    assert( s->type >= STRING_TYPE_START );

    object_simple *ss = (object_simple*)s;
    switch (s->type) {
        case GC_STRING:
        case GC_ROPE:
        case GC_STRINGC:
        return ((string_header *)ss->simple_data.ptr)->chars;

        case GC_SNIP:
        return ss->simple_data.snip.chars;
    }

    assert(0);
}

int string_compare( object *a, object *b ) {
    assert( a->ref_count && b->ref_count );
    assert( a->type >= STRING_TYPE_START );
    assert( b->type >= STRING_TYPE_START );

    string_iterator *ia = iterate_string(a);
    string_iterator *ib = iterate_string(b);

    while (!(iterator_complete(ia) || iterator_complete(ib))) { 
        uint8_t ca = iterator_next_byte(ia);
        uint8_t cb = iterator_next_byte(ib);

        if (ca == cb) continue;
        if (ca >  cb) return  1;
        if (ca <  cb) return -1;
    }

    if (iterator_complete(ia) && iterator_complete(ib))
        return 0;

    if (iterator_complete(ia)) return -1;
    if (iterator_complete(ib)) return  1;

    // Cannot happen
    assert(0);
}

string_iterator *iterate_string( object *s ) {
    assert( s->ref_count );
    assert( s->type >= STRING_TYPE_START );

    int32_t depth = 0;
    if ( s->type == GC_ROPE ) {
        string_rope *r = ((object_simple *)(s))->simple_data.ptr;
        depth = r->depth;
    }
    string_iterator *i = malloc(sizeof(string_iterator) + depth);
    assert( i );
    memset( i, 0, sizeof(string_iterator) + depth );
    gc_add_ref(s);
    i->stack.item = (object_simple*)s;

    return i;
}

void free_string_iterator( string_iterator *i ) {
    gc_del_ref(&(i->stack.item->object));
    free(i);
}

const uint8_t *iterator_next_part( string_iterator *i, ucs4_t *c ) {
    while( 1 ) {
        if (i->complete) return NULL;

        size_t         total_bytes;
        const uint8_t *out;
        string        *st;
        string_rope   *sr;
        string_snip   *sn;
        string_const  *sc;

        string_iterator_stack *stack = &(i->stack);
        string_iterator_stack *curr  = stack + i->stack_index;

        switch (curr->item->object.type) {
            case GC_ROPE:
                sr = curr->item->simple_data.ptr;

                // Pop if there is nothing left here.
                if (curr->index >= sr->child_count) {
                    assert( i->stack_index > 0 );
                    i->stack_index--;
                    continue;
                }

                // Push
                curr[1].item  = sr->children[curr->index];
                curr[1].index = 0;
                (i->stack_index)++;
                (curr->index)++;

                total_bytes = sr->child_count;
            continue;

            case GC_STRING:
                st = i->stack.item->simple_data.ptr;
                out = &(st->first_byte) + i->stack.index;
                total_bytes = st->head.bytes;
            break;

            case GC_SNIP:
                sn = &(curr->item->simple_data.snip);
                out = sn->data + curr->index;
                total_bytes = sn->bytes;
            break;

            case GC_STRINGC:
                sc = curr->item->simple_data.ptr;
                out = sc->string + curr->index;
                total_bytes = sc->head.bytes;
            break;

            default:
                assert(0);
        }

        if (c) {
            (curr->index) += u8_mbtouc(c, out, total_bytes - i->stack.index);
        }
        else {
            (curr->index)++;
        }

        if (curr->index >= total_bytes) {
            if (i->stack_index) {
                stack = &(i->stack);
                i->stack_index--;
                if (!i->stack_index) {
                    sr = i->stack.item->simple_data.ptr;
                    if (i->stack.index >= sr->child_count) {
                        i->complete = 1;
                    }
                }
            }
            else {
                i->complete = 1;
            }
        }

        return out;
    }
}

uint8_t iterator_next_byte( string_iterator *i ) {
    if (i->complete) return 0;
    if (i->units == I_ANY) i->units = I_BYTES;
    assert( i->units == I_BYTES );
    const uint8_t *x = (iterator_next_part(i, NULL));
    if (!x) return 0;
    return *x;
}

ucs4_t iterator_next_unic( string_iterator *i ) {
    if (i->complete) return 0;
    if (i->units == I_ANY) i->units = I_CHARS;
    assert( i->units == I_CHARS );

    ucs4_t c;
    iterator_next_part(i, &c );
    return c;
}

void free_string( object *s ) {
    assert( s->type >= STRING_TYPE_START );

    object_simple *ss = (object_simple*)s;
    switch (s->type) {
        case GC_SNIP: return; //nothing to do

        case GC_STRINGC:  // String part is constant, kill the pointed structure
        case GC_ROPE:     // data is constrained to pointer and other objects
        case GC_STRING:   // String is contained in the pointer
            free( ss->simple_data.ptr );
        return;
    }

    // This will never happen, but the compiler can't tell.
    assert(0);
}

uint8_t iterator_complete( string_iterator *i ) {
    return i->complete;
}

int is_string( object *o ) {
    return o->type >= STRING_TYPE_START;
}
