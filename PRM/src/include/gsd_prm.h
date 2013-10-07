#ifndef GSD_PRM_H
#define GSD_PRM_H

#include <stdint.h>
#include <stdlib.h>

typedef struct prm        prm;
typedef struct destructor destructor;

struct destructor {
    void (*callback)( void *mem, void *arg );
    void *arg;
};

prm *build_prm( uint8_t epochs, size_t epoch_size, size_t fork_at, destructor *d );
void free_prm( prm *c );

uint8_t join_epoch( prm *c );
void   leave_epoch( prm *c, uint8_t epoch );

int dispose( prm *p, void *garbage );

#endif
