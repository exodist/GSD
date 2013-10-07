#ifndef GSD_DICT_EPOCH
#define GSD_DICT_EPOCH

typedef struct epoch_set  epoch_set;
typedef struct epoch      epoch;
typedef struct destructor destructor;

struct destructor {
    void (*callback)( void *mem, void *arg );
    void *arg;
};

epoch_set *build_epoch_set( uint8_t epochs, size_t compactor_size );
void free_epoch_set( epoch_set *s );

epoch *join_epoch( epoch_set *set );
void leave_epoch( epoch_set *set, epoch *e );

int dispose( epoch_set *s, void *garbage, destructor *d );

#endif
