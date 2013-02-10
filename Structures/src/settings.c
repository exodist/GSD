#include "include/gsd_dict.h"
#include "include/gsd_dict_return_old.h"

#include "settings.h"
#include "structure.h"

dict_settings *get_settings( dict *d ) {
    return d->set->settings;
}

dict_methods *get_methods( dict *d ) {
    return d->methods;
}

int reconfigure( dict *d, dict_settings *settings ) {
    return DICT_UNIMP_ERROR;
}

