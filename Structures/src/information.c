#include "information.h"

//-------------------
// These functions are defined in gsd_dict.h
// They are publicly exposed functions.
// Changing how these work requires a major version bump.
//-------------------

dict_settings *dict_get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods *dict_get_methods( dict *d ) {
    return d->methods;
}

//------------------------------------------------
// Nothing below here is publicly exposed.
//------------------------------------------------
