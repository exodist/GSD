#ifndef GSD_PRM_H
#define GSD_PRM_H

#include <stdint.h>
#include <stdlib.h>

typedef struct prm        prm;
typedef struct destructor destructor;

/*\
 * epochs     - How many epochs to have (they cycle, 2 is usually the minimum)
 * epoch_size - How much garbage can build up before epoch change is forced.
                (It is possible garbage will be freed before this limit is
                reached.)
 * thread_at  - Use a parallel thread to free the garbage if there is this much
                garbage. If the garbage is below this it will not thread. 0
                means never thread.
\*/
prm *build_prm( uint8_t epochs, uint8_t epoch_size, size_t thread_at );
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

/*\ Documentation

First you must create a prm using build_prm. Most things will need at least 2
epochs. epoch size must be (7 < size < 65), if you specify something outside
this range it will bump it up or down to min/max.

Each thread must join an epoch before it obtains any pointers to data that
could be disposed of by other threads. When the thread is done with all its
pointers it call leave_epoch.

When a thread takes action that means no other thread can gain a reference to
some data, dispose() should be called on that data. It is OK if other threads
already have a reference to the data, but it must not be possible for them to
gain a new one.

When all threads leave the epoch in which the data was disposed, it will be
freed.

EXAMPLE:

    void *get_ref();
    void remove_ref(void *);

    uint8_t e = join_epoch( p );
    void *ref = get_ref();

    // After this no other thread will be able to get the ref, though it is
    // possible they already have it.
    if(remove_ref(ref)) { // if it fails then another thread removed it
        // This is safe, it will not be freed until all threads leave the epoch.
        dispose( p, ref, NULL, NULL );
    }

    leave_epoch( p, e );

\*/

#endif
