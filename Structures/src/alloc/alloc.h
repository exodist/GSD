#ifndef ALLOC_ALLOC_H
#define ALLOC_ALLOC_H

#include "../include/gsd_struct_ref.h"

struct alloc_chunk {
    
};

struct alloc {
    size_t item_size;
    size_t chunk_size;
    refdelta *rd;

    size_t        chunk_count;
    alloc_chunk  *chunks;
    bitmap      **chunk_maps;

    // UHG...
    size_t  consumer_count;
    size_t  consumer_size;
    bitmap *consumer_map;
    bloom **consumer_filters        
};

#endif
