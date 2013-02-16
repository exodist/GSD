#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include "include/gsd_dict_return.h"
#include "error.h"

// rstat stands for return_stat
typedef dict_stat rstat;

extern char *error_messages[];

char *error_message( dict_stat s );
rstat error( uint8_t fail, uint8_t rebal, uint8_t cat, uint8_t midx );

extern rstat rstat_ok;
extern rstat rstat_mem;
extern rstat rstat_trans;
extern rstat rstat_patho;
extern rstat rstat_unimp;


#endif
