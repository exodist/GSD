#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include "include/gsd_dict_return.h"
#include "error.h"

// rstat stands for return_stat
typedef dict_stat rstat;

#define error( f, r, c, m, i ) make_error( f, r, c, m, i, __LINE__, __FILE__ )

const char *error_message( dict_stat s );
rstat make_error( uint8_t fail, uint8_t rebal, uint8_t cat, const char *msg, uint8_t i, size_t ln, const char *fn );

extern rstat rstat_ok;
extern rstat rstat_mem;
extern rstat rstat_trans;
extern rstat rstat_patho;
extern rstat rstat_unimp;
extern rstat rstat_imute;
extern rstat rstat_trigg;


#endif
