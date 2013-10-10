#ifndef GSD_PRM_H
#define GSD_PRM_H

#include <stdint.h>
#include <stdlib.h>

typedef struct prm        prm;
typedef struct destructor destructor;

prm *build_prm( uint8_t epochs, uint8_t epoch_size, size_t fork_at );
int free_prm( prm *p );

uint8_t join_epoch( prm *p );
void   leave_epoch( prm *p, uint8_t epoch );

int dispose(
    prm *p,
    void *garbage,
    // These are optional, but if 'arg' is specified 'destroy' must be as well.
    // However arg is ALWAYS optional, when specified it is passed to 'destroy'.
    void (*destroy)(void *ptr, void *arg),
    void *arg
);

#endif
