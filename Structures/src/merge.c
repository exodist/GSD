#include "structure.h"

//-------------------
// These functions are defined in gsd_dict.h
// They are publicly exposed functions.
// Changing how these work requires a major version bump.
//-------------------

int merge( dict *from, dict *to );
int merge_refs( dict *from, dict *to );

//------------------------------------------------
// Nothing below here is publicly exposed.
//------------------------------------------------

