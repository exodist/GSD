#include "include/gsd_dict.h"

#include "settings.h"
#include "structure.h"

dict_settings *get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods *get_methods( dict *d ) {
    return d->methods;
}

rstat reconfigure( dict *d, dict_settings *settings ) {
    return rstat_unimp;
}

