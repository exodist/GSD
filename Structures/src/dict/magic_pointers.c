#include "magic_pointers.h"

char blockers[3];
const void *RBLD = &(blockers[0]);
const void *IMUT = &(blockers[1]);
const void *RBAL = &(blockers[2]);
