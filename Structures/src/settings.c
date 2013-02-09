#include "include/gsd_dict.h"
#include "include/gsd_dict_return.h"

#include "settings.h"
#include "structure.h"

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

int dict_reconfigure( dict *d, dict_settings *settings ) {
    return DICT_UNIMP_ERROR;
}

//------------------------------------------------
// Nothing below here is publicly exposed.
//------------------------------------------------

