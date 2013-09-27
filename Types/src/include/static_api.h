#ifndef GSD_GC_STATIC_API_H
#define GSD_GC_STATIC_API_H

typedef struct object object;

extern object *O_TRUE;
extern object *O_FALSE;
extern object *O_UNDEF;
extern object *O_ZERO;
extern object *O_PINTS; // has 1 ->  127
extern object *O_NINTS; // has 1 -> -127

#endif
